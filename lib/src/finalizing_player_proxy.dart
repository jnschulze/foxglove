import 'package:foxglove/src/base.dart';
import 'package:foxglove/src/enums.dart';
import 'package:foxglove/src/media/media_source.dart';
import 'package:foxglove/src/player_platform.dart';
import 'package:foxglove/src/player_state.dart';
import 'package:logging/logging.dart';

class FinalizingPlayerProxy implements Player {
  static final _logger = Logger('foxglove:FinalizingPlayerProxy');
  static final _finalizer = Finalizer<Player>((player) async {
    try {
      _logger.fine('Attempting to dispose player');
      await player.dispose();
      _logger.fine('Player disposed');
    } catch (e) {
      _logger.severe('Disposing player failed', e);
    }
  });

  /// The backing [Player].
  final Player _player;

  FinalizingPlayerProxy(Player backingPlayer) : _player = backingPlayer {
    _finalizer.attach(this, backingPlayer, detach: this);
  }

  @override
  bool get isDisposed => _player.isDisposed;

  @override
  int get id => _player.id;

  @override
  int get textureId => _player.textureId;

  @override
  CurrentMediaState get currentMedia => _player.currentMedia;

  @override
  Stream<CurrentMediaState> get currentMediaStream =>
      _player.currentMediaStream;

  @override
  GeneralState get general => _player.general;

  @override
  Stream<GeneralState> get generalStream => _player.generalStream;

  @override
  Future<void> mute() => _player.mute();

  @override
  Future<void> open(MediaSource media, {bool autoStart = true}) =>
      _player.open(media, autoStart: autoStart);

  @override
  Future<void> close() => _player.close();

  @override
  Future<void> pause() => _player.pause();

  @override
  Future<void> play() => _player.play();

  @override
  PlaybackStatus get playback => _player.playback;

  @override
  Stream<PlaybackStatus> get playbackStream => _player.playbackStream;

  @override
  PositionState get position => _player.position;

  @override
  Stream<PositionState> get positionStream => _player.positionStream;

  @override
  Future<void> seekPosition(double position) => _player.seekPosition(position);

  @override
  Future<void> seekTime(Duration time) => _player.seekTime(time);

  @override
  Future<void> setLoopMode(LoopMode loopMode) => _player.setLoopMode(loopMode);

  @override
  Future<void> setPositionReportingEnabled(bool flag) =>
      _player.setPositionReportingEnabled(flag);

  @override
  Future<void> setRate(double rate) => _player.setRate(rate);

  @override
  Future<void> setVolume(double volume) => _player.setVolume(volume);

  @override
  Future<void> stop() => _player.stop();

  @override
  Future<void> unmute() => _player.unmute();

  @override
  VideoDimensions get videoDimensions => _player.videoDimensions;

  @override
  Stream<VideoDimensions> get videoDimensionsStream =>
      _player.videoDimensionsStream;

  @override
  Future<void> dispose() {
    _finalizer.detach(this);
    return _player.dispose();
  }
}
