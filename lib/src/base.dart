/// Represents dimensions of a video.
class VideoDimensions {
  /// Width of the video.
  final int width;

  /// Height of the video.
  final int height;

  const VideoDimensions({this.width = 0, this.height = 0});

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is VideoDimensions &&
        other.width == width &&
        other.height == height;
  }

  @override
  int get hashCode => Object.hash(width, height);

  @override
  String toString() => '($width, $height)';
}
