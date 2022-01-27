import 'package:foxglove/src/enums.dart';
import 'package:foxglove/src/internal/utils.dart';
import 'package:foxglove/src/media/media.dart';
import 'package:foxglove/src/media/media_source.dart';

class Playlist implements MediaSource {
  @override
  MediaSourceType get mediaSourceType => MediaSourceType.playlist;

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
