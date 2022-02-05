import 'dart:async';

import 'package:flutter/services.dart';
import 'package:foxglove/foxglove.dart';

enum _PlatformEvent {
  none,
  positionChanged,
  playbackStateChanged,
  mediaChanged,
  rateChanged,
  volumeChanged,
  muteChanged,
  videoDimensionsChanged
}

mixin _StreamControllers {
  final _videoDimensionsStreamController =
      StreamController<VideoDimensions>.broadcast();
  final _currentMediaStateController =
      StreamController<CurrentMediaState>.broadcast();
  final _positionStateController = StreamController<PositionState>.broadcast();
  final _playbackStateController = StreamController<PlaybackStatus>.broadcast();
  final _generalStateController = StreamController<GeneralState>.broadcast();
  final _videoFrameController = StreamController<VideoFrame>.broadcast();

  CurrentMediaState _currentMediaState = const CurrentMediaState();
  PositionState _positionState = const PositionState();
  PlaybackStatus _playbackState = const PlaybackStatus();
  GeneralState _generalState = const GeneralState();
  VideoDimensions _videoDimensions = const VideoDimensions();

  void _closeControllers() {
    _videoDimensionsStreamController.close();
    _videoFrameController.close();
    _currentMediaStateController.close();
    _positionStateController.close();
    _playbackStateController.close();
    _generalStateController.close();
  }
}

extension on Media {
  Map<String, dynamic> toJson() {
    return {'type': mediaType.name, 'resource': resource};
  }
}

extension on Playlist {
  Map<String, dynamic> toJson() {
    final medias = this.medias.map((m) => m.toJson()).toList();
    return {'medias': medias, 'mode': playlistMode.index};
  }
}

typedef DisposeCallback = Future<void> Function();

class MethodChannelPlayer extends PlayerPlatform {
  static const MethodChannel _channel = MethodChannel('foxglove');

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
    final result = await _channel.invokeMapMethod(
        'createPlayer', <String, dynamic>{
      'environmentId': environmentId,
      'environmentArgs': environmentArgs
    });

    final playerId = result!['player_id'] as int;
    final textureId = result['texture_id'] as int;
    return _PlayerImpl(
        platform: this,
        id: playerId,
        textureId: textureId,
        disposeCallback: () async {
          await _disposePlayer(playerId);
        });
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

  @override
  Stream<VideoFrame> get videoFrameStream => _videoFrameController.stream;

  @override
  Future<void> open(MediaSource source, {bool autoStart = true}) async {
    if (_isDisposed) {
      return;
    }

    if (source is Media) {
      await _methodChannel.invokeMethod(
          'open', {'media': source.toJson(), 'autostart': autoStart});
    } else if (source is Playlist) {
      await _methodChannel.invokeMethod(
          'open', {'playlist': source.toJson(), 'autostart': autoStart});
    }
  }

  @override
  Future<void> setPlaylistMode(PlaylistMode mode) async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('setPlaylistMode', mode.index);
  }

  @override
  Future<void> play() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('play');
  }

  @override
  Future<void> pause() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('pause');
  }

  @override
  Future<void> stop() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('stop');
  }

  @override
  Future<void> next() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('next');
  }

  @override
  Future<void> previous() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('previous');
  }

  @override
  Future<void> seekPosition(double position) async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('seekPosition', position);
  }

  @override
  Future<void> seekTime(Duration position) async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('seekTime', position.inMilliseconds);
  }

  @override
  Future<void> setRate(double rate) async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('setRate', rate);
  }

  @override
  Future<void> setVolume(double volume) async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('setVolume', volume);
  }

  @override
  Future<void> mute() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('mute');
  }

  @override
  Future<void> unmute() async {
    if (_isDisposed) {
      return;
    }
    await _methodChannel.invokeMethod('unmute');
  }

  @override
  Future<void> dispose() async {
    if (!_isDisposed) {
      _isDisposed = true;
      _closeControllers();
      _eventChannelSubscription?.cancel();
      await _disposeCallback();
    }
  }

  void _handlePlatformEvent(Map<dynamic, dynamic> ev) {
    if (_isDisposed) {
      return;
    }

    final event = _PlatformEvent.values[ev['type'] as int];

    switch (event) {
      case _PlatformEvent.positionChanged:
        final position = ev['position'] as double;
        final duration = ev['duration'] as int;
        assert(position >= 0 && position <= 1);
        _positionState = _positionState.copyWith(
            relativePosition: position,
            duration: Duration(milliseconds: duration));
        _positionStateController.sink.add(_positionState);
        break;
      case _PlatformEvent.playbackStateChanged:
        final stateIndex = ev['state'] as int;
        final state = PlaybackState.values[stateIndex];
        final isSeekable = ev['is_seekable'] as bool;
        _playbackState = _playbackState.copyWith(
            playbackState: state, isSeekable: isSeekable);
        _playbackStateController.sink.add(_playbackState);
        break;
      case _PlatformEvent.mediaChanged:
        final media = Media.fromJson(ev['media']);
        final index = ev['index'];
        _currentMediaState =
            _currentMediaState.copyWith(media: media, index: index);
        _currentMediaStateController.sink.add(_currentMediaState);
        break;
      case _PlatformEvent.rateChanged:
        final rate = ev['value'] as double;
        _generalState = _generalState.copyWith(rate: rate);
        _generalStateController.sink.add(_generalState);
        break;
      case _PlatformEvent.volumeChanged:
        final value = ev['value'] as double;
        assert(value >= 0 && value <= 1);
        _generalState = _generalState.copyWith(volume: value);
        _generalStateController.sink.add(_generalState);
        break;
      case _PlatformEvent.muteChanged:
        final value = ev['value'] as bool;
        _generalState = _generalState.copyWith(isMuted: value);
        _generalStateController.sink.add(_generalState);
        break;
      case _PlatformEvent.videoDimensionsChanged:
        final width = ev['width'] as int;
        final height = ev['height'] as int;
        _videoDimensions = VideoDimensions(width: width, height: height);
        _videoDimensionsStreamController.sink.add(_videoDimensions);
        break;
      default:
        print('Got unknown event');
    }
  }
}
