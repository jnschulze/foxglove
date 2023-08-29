/// Represents dimensions of a video.
class VideoDimensions {
  /// Width of the video.
  final int width;

  /// Height of the video.
  final int height;
  const VideoDimensions({this.width = 0, this.height = 0});

  @override
  String toString() => '($width, $height)';
}
