import 'package:foxglove/src/enums/media_source_type.dart';
import 'package:foxglove/src/enums/playlist_mode.dart';
import 'package:foxglove/src/internal/utils.dart';
import 'package:foxglove/src/media_source/media.dart';
import 'package:foxglove/src/media_source/media_source.dart';

class Playlist implements MediaSource {
  @override
  MediaSourceType get mediaSourceType => MediaSourceType.playlist;

  /// [List] of [Media] present in the playlist.
  final List<Media> medias;
  final PlaylistMode playlistMode;

  const Playlist(
      {required this.medias, this.playlistMode = PlaylistMode.single});

  @override
  int get hashCode => medias.hashCode ^ playlistMode.hashCode;

  @override
  bool operator ==(Object other) =>
      other is Playlist &&
      other.playlistMode == playlistMode &&
      listEquals(other.medias, medias);

  @override
  String toString() => 'Playlist[${medias.length}]';
}
