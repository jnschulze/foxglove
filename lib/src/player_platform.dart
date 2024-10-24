import 'package:foxglove/src/base.dart';
import 'package:foxglove/src/enums.dart';
import 'package:foxglove/src/media/media_source.dart';
import 'package:foxglove/src/method_channel_player.dart';
import 'package:foxglove/src/platform_log_config.dart';
import 'package:foxglove/src/player_state.dart';

abstract interface class Player {
  // Player id.
  int get id;

  // Texture id.
  int get textureId;

  bool get isDisposed;

  /// State of the current & opened [MediaSource] in [Player] instance.
  CurrentMediaState get currentMedia;

  /// Stream to listen to current & opened [MediaSource] state of the [Player] instance.
  Stream<CurrentMediaState> get currentMediaStream;

  /// Position & duration state of the [Player] instance.
  PositionState get position;

  /// Stream to listen to position & duration state of the [Player] instance.
  Stream<PositionState> get positionStream;

  /// Playback state of the [Player] instance.
  PlaybackStatus get playback;

  /// Stream to listen to playback state of the [Player] instance.
  Stream<PlaybackStatus> get playbackStream;

  /// Volume & Rate state of the [Player] instance.
  GeneralState get general;

  /// Stream to listen to volume & rate state of the [Player] instance.
  Stream<GeneralState> get generalStream;

  /// Dimensions of the currently playing video.
  VideoDimensions get videoDimensions;

  /// Stream to listen to dimensions of currently playing video.
  Stream<VideoDimensions> get videoDimensionsStream;

  /// Opens the given [MediaSource].
  Future<void> open(MediaSource media, {bool autoStart = true});

  /// Closes the currently open [MediaSource], if any.
  Future<void> close();

  Future<void> setLoopMode(LoopMode loopMode);
  Future<void> setPositionReportingEnabled(bool flag);
  Future<void> play();
  Future<void> pause();
  Future<void> stop();
  Future<void> seekPosition(double position);
  Future<void> seekTime(Duration time);
  Future<void> setRate(double rate);
  Future<void> setVolume(double volume);
  Future<void> mute();
  Future<void> unmute();
  Future<void> dispose();
}

abstract class PlayerPlatform {
  static PlayerPlatform _instance = MethodChannelPlayer();

  /// The default instance of [PlayerPlatform] to use.
  ///
  /// Defaults to [MethodChannelPlayer].
  // ignore: unnecessary_getters_setters
  static PlayerPlatform get instance => _instance;

  /// Platform-specific plugins should override this with their own
  /// platform-specific class that extends [PlayerPlatform] when they
  /// register themselves.
  static set instance(PlayerPlatform instance) {
    _instance = instance;
  }

  Future<void> initialize();

  /// Configures native logging.
  /// This function can be called multiple times.
  Future<void> configureLogging(PlatformLogConfig logConfig);

  /// Attempts to create an enviroment with the given [args]
  Future<int> createEnvironment({List<String>? args});

  /// Disposes the environment with the given [environmentId].
  Future<void> disposeEnvironment(int environmentId);

  /// Attempts to create a new player.
  Future<Player> createPlayer(
      {int? environmentId, List<String>? environmentArgs});
}
