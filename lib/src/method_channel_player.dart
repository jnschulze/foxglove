import 'dart:async';

import 'package:flutter/services.dart';
import 'package:foxglove/foxglove.dart';
import 'package:foxglove/src/finalizing_player_proxy.dart';
import 'package:logging/logging.dart';

enum _PlatformEvent {
  none,
  initialized,
  positionChanged,
  playbackStateChanged,
  mediaChanged,
  rateChanged,
  volumeChanged,
  muteChanged,
  videoDimensionsChanged,
  isSeekableChanged
}

mixin _StreamControllers {
  final _videoDimensionsStreamController =
      StreamController<VideoDimensions>.broadcast();
  final _currentMediaStateController =
      StreamController<CurrentMediaState>.broadcast();
  final _positionStateController = StreamController<PositionState>.broadcast();
  final _playbackStateController = StreamController<PlaybackStatus>.broadcast();
  final _generalStateController = StreamController<GeneralState>.broadcast();

  CurrentMediaState _currentMediaState = const CurrentMediaState();
  PositionState _positionState = const PositionState();
  PlaybackStatus _playbackState = const PlaybackStatus();
  GeneralState _generalState = const GeneralState();
  VideoDimensions _videoDimensions = const VideoDimensions();

  void _closeControllers() {
    _videoDimensionsStreamController.close();
    _currentMediaStateController.close();
    _positionStateController.close();
    _playbackStateController.close();
    _generalStateController.close();
  }
}

typedef DisposeCallback = Future<void> Function();

class MethodChannelPlayer extends PlayerPlatform {
  static final _logger = Logger('foxglove:MethodChannelPlayer');
  static const MethodChannel _channel = MethodChannel('foxglove');

  @override
  Future<void> initialize() => _channel.invokeMethod('init');

  @override
  Future<void> configureLogging(PlatformLogConfig logConfig) async {
    await _channel.invokeMethod('configureLogging', <String, dynamic>{
      'enableConsoleLogging': logConfig.enableConsoleLogging,
      'consoleLogLevel': logConfig.consoleLogLevel?.index,
      'fileLogPath': logConfig.fileLogPath,
      'fileLogLevel': logConfig.fileLogLevel?.index
    });
  }

  @override
  Future<int> createEnvironment({List<String>? args}) async {
    final envId = await _channel.invokeMethod<int>(
        'createEnvironment', <String, dynamic>{'args': args}) as int;
    return envId;
  }

  @override
  Future<void> disposeEnvironment(int environmentId) async {
    await _channel.invokeMethod('disposeEnvironment', environmentId);
  }

  @override
  Future<Player> createPlayer(
      {int? environmentId, List<String>? environmentArgs}) async {
    try {
      _logger.fine(
          'Creating player with environmentId `$environmentId` and args `$environmentArgs`');
      final result = await _channel.invokeMapMethod(
          'createPlayer', <String, dynamic>{
        'environmentId': environmentId,
        'environmentArgs': environmentArgs
      });

      final playerId = result!['player_id'] as int;
      final textureId = result['texture_id'] as int;

      _logger.fine('Created player with id: $playerId');
      final player = _PlayerImpl(
          platform: this,
          id: playerId,
          textureId: textureId,
          disposeCallback: () async {
            await _disposePlayer(playerId);
          });
      return FinalizingPlayerProxy(player);
    } catch (e) {
      _logger.severe('Error creating player: $e', e);
      rethrow;
    }
  }

  Future<void> _disposePlayer(int id) async {
    await _channel.invokeMethod('disposePlayer', id);
  }
}

class _PlayerImpl with _StreamControllers implements Player {
  final PlayerPlatform platform;

  @override
  final int id;

  @override
  final int textureId;

  @override
  bool get isDisposed => _isDisposed;

  final DisposeCallback _disposeCallback;
  final MethodChannel _methodChannel;
  final EventChannel _eventChannel;
  StreamSubscription? _eventChannelSubscription;
  bool _isDisposed = false;
  final Logger _logger = Logger('foxglove:Player');
  final _eventChannelReady = Completer<void>();

  _PlayerImpl(
      {required this.platform,
      required this.id,
      required this.textureId,
      required DisposeCallback disposeCallback})
      : _disposeCallback = disposeCallback,
        _methodChannel = MethodChannel('foxglove/$id'),
        _eventChannel = EventChannel('foxglove/$id/events') {
    _eventChannelSubscription = _eventChannel.receiveBroadcastStream().listen(
        (event) => _handlePlatformEvent(event as Map<dynamic, dynamic>));
  }

  @override
  CurrentMediaState get currentMedia => _currentMediaState;

  @override
  Stream<CurrentMediaState> get currentMediaStream =>
      _currentMediaStateController.stream;

  @override
  PositionState get position => _positionState;

  @override
  Stream<PositionState> get positionStream => _positionStateController.stream;

  @override
  PlaybackStatus get playback => _playbackState;

  @override
  Stream<PlaybackStatus> get playbackStream => _playbackStateController.stream;

  @override
  GeneralState get general => _generalState;

  @override
  Stream<GeneralState> get generalStream => _generalStateController.stream;

  @override
  VideoDimensions get videoDimensions => _videoDimensions;

  @override
  Stream<VideoDimensions> get videoDimensionsStream =>
      _videoDimensionsStreamController.stream;

  Future<T?> _invokeMethod<T>(String methodName, [dynamic arguments]) async {
    if (_isDisposed) {
      _logger.warning(
          'Player is already disposed, ignoring $methodName method call.');
      return null;
    }
    await _eventChannelReady.future;

    _logger.fine('Invoking $methodName');
    final result = await _methodChannel.invokeMethod<T>(methodName, arguments);
    _logger.fine('Invoked $methodName');
    return result;
  }

  @override
  Future<void> open(MediaSource source, {bool autoStart = true}) async {
    _logger.info('Opening media: $source');
    await _invokeMethod(
        'open', {'media': source.toJson(), 'autostart': autoStart});
  }

  @override
  Future<void> close() => _invokeMethod('close');

  @override
  Future<void> setLoopMode(LoopMode mode) =>
      _invokeMethod('setLoopMode', mode.index);

  @override
  Future<void> setPositionReportingEnabled(bool flag) =>
      _invokeMethod('setPositionReportingEnabled', flag);

  @override
  Future<void> play() => _invokeMethod('play');

  @override
  Future<void> pause() => _invokeMethod('pause');

  @override
  Future<void> stop() => _invokeMethod('stop');

  @override
  Future<void> seekPosition(double position) =>
      _invokeMethod('seekPosition', position);

  @override
  Future<void> seekTime(Duration position) =>
      _invokeMethod('seekTime', position.inMilliseconds);

  @override
  Future<void> setRate(double rate) => _invokeMethod('setRate', rate);

  @override
  Future<void> setVolume(double volume) => _invokeMethod('setVolume', volume);

  @override
  Future<void> mute() => _invokeMethod('mute');

  @override
  Future<void> unmute() => _invokeMethod('unmute');

  @override
  Future<void> dispose() async {
    if (!_isDisposed) {
      _logger.info('Disposing player');
      _isDisposed = true;
      _closeControllers();
      _eventChannelSubscription?.cancel();
      await _disposeCallback();
    }
  }

  void _handlePlatformEvent(Map<dynamic, dynamic> ev) {
    if (_isDisposed) {
      _logger.warning('Player is already disposed, ignoring event');
      return;
    }

    final event = _PlatformEvent.values[ev['type'] as int];
    switch (event) {
      case _PlatformEvent.initialized:
        _eventChannelReady.complete();
        break;
      case _PlatformEvent.positionChanged:
        final position = ev['position'] as double;
        final duration = ev['duration'] as int;
        _logger.finest('Position changed: $position');

        assert(position >= 0 && position <= 1);
        _positionState = _positionState.copyWith(
            relativePosition: position,
            duration: Duration(milliseconds: duration));
        _positionStateController.sink.add(_positionState);
        break;
      case _PlatformEvent.playbackStateChanged:
        final stateIndex = ev['state'] as int;
        final state = PlaybackState.values[stateIndex];
        _logger.info('Playback state changed: $state');
        _playbackState = _playbackState.copyWith(playbackState: state);
        _playbackStateController.sink.add(_playbackState);
        break;
      case _PlatformEvent.isSeekableChanged:
        final isSeekable = ev['value'] as bool;
        _logger.info('isSeekable changed: $isSeekable');
        _playbackState = _playbackState.copyWith(isSeekable: isSeekable);
        _playbackStateController.sink.add(_playbackState);
        break;
      case _PlatformEvent.mediaChanged:
        final rawMedia = ev['media'];
        final media = rawMedia != null ? Media.fromJson(rawMedia) : null;
        _logger.info('Media changed: ${media?.resource}');
        _currentMediaState = CurrentMediaState(media: media);
        _currentMediaStateController.sink.add(_currentMediaState);
        break;
      case _PlatformEvent.rateChanged:
        final rate = ev['value'] as double;
        _logger.info('Rate changed: $rate');
        _generalState = _generalState.copyWith(rate: rate);
        _generalStateController.sink.add(_generalState);
        break;
      case _PlatformEvent.volumeChanged:
        final value = ev['value'] as double;
        _logger.info('Volume changed: $value');
        assert(value >= 0 && value <= 1);
        _generalState = _generalState.copyWith(volume: value);
        _generalStateController.sink.add(_generalState);
        break;
      case _PlatformEvent.muteChanged:
        final value = ev['value'] as bool;
        _logger.info('Mute changed: $value');
        _generalState = _generalState.copyWith(isMuted: value);
        _generalStateController.sink.add(_generalState);
        break;
      case _PlatformEvent.videoDimensionsChanged:
        final width = ev['width'] as int;
        final height = ev['height'] as int;
        _logger.info('Video dimensions changed: $width x $height');
        _videoDimensions = VideoDimensions(width: width, height: height);
        _videoDimensionsStreamController.sink.add(_videoDimensions);
        break;
      default:
        _logger.warning('Unknown event type: $event');
    }
  }
}
