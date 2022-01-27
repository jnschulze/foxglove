import 'dart:typed_data';

enum PixelFormat { kNone, kRGBA, kBGRA }

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

/// Represents a [Video] frame, used for retriving frame through platform channel.
class VideoFrame {
  final int width;
  final int height;
  final Uint8List buffer;
  final int bytesPerRow;
  final PixelFormat pixelFormat;

  VideoFrame({
    required this.width,
    required this.height,
    required this.bytesPerRow,
    required this.buffer,
    required this.pixelFormat,
  });
}
