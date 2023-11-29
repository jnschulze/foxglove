import 'package:foxglove/src/enums.dart';
import 'package:foxglove/src/media/media.dart';

/// State of a [Player] instance.
class CurrentMediaState {
  /// Currently playing [Media].
  final Media? media;

  const CurrentMediaState({this.media});

  CurrentMediaState copyWith({Media? media}) => CurrentMediaState(
        media: media ?? this.media,
      );
}

/// Position & duration state of a [Player] instance.
class PositionState {
  final double relativePosition;

  /// Position of playback in [Duration] of currently playing [Media].
  Duration get position => duration * relativePosition;

  /// Length of currently playing [Media] in [Duration].
  final Duration duration;

  const PositionState(
      {this.relativePosition = 0.0, this.duration = Duration.zero});

  PositionState copyWith({double? relativePosition, Duration? duration}) =>
      PositionState(
          relativePosition: relativePosition ?? this.relativePosition,
          duration: duration ?? this.duration);
}

/// Playback state of a [Player] instance.
class PlaybackStatus {
  final PlaybackState playbackState;

  /// Whether [Player] instance is seekable or not.
  final bool isSeekable;

  /// Whether [Player] instance is playing or not.
  bool get isPlaying => playbackState == PlaybackState.playing;

  /// Whether the current [Media] has ended playing or not.
  bool get isCompleted => playbackState == PlaybackState.ended;

  const PlaybackStatus(
      {this.playbackState = PlaybackState.none, this.isSeekable = false});

  PlaybackStatus copyWith({PlaybackState? playbackState, bool? isSeekable}) =>
      PlaybackStatus(
          playbackState: playbackState ?? this.playbackState,
          isSeekable: isSeekable ?? this.isSeekable);
}

/// Volume & Rate state of a [Player] instance.
class GeneralState {
  /// Volume of [Player] instance.
  final double volume;

  /// Whether the player is muted.
  final bool isMuted;

  /// Rate of playback of [Player] instance.
  final double rate;

  const GeneralState(
      {this.volume = 1.0, this.isMuted = false, this.rate = 1.0});

  GeneralState copyWith({double? volume, bool? isMuted, double? rate}) =>
      GeneralState(
          volume: volume ?? this.volume,
          isMuted: isMuted ?? this.isMuted,
          rate: rate ?? this.rate);
}
