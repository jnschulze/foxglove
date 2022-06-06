import 'dart:async';
import 'dart:io';
import 'dart:ui' as ui;
import 'package:flutter/material.dart';
import 'package:foxglove/src/base.dart';
import 'package:foxglove/src/player_platform.dart';

final bool hasTextureSupport = Platform.isWindows;

extension ImageExtensions on VideoFrame {
  Future<ui.Image> toImage() {
    Completer<ui.Image> imageCompleter = Completer<ui.Image>();
    ui.decodeImageFromPixels(
        buffer,
        width,
        height,
        pixelFormat == PixelFormat.kRGBA
            ? ui.PixelFormat.rgba8888
            : ui.PixelFormat.bgra8888,
        (ui.Image _image) => imageCompleter.complete(_image),
        rowBytes: bytesPerRow);
    return imageCompleter.future;
  }
}

class Video extends StatefulWidget {
  final Player player;

  /// Width of the viewport.
  /// Defaults to the width of the parent.
  final double? width;

  /// Height of the viewport.
  /// Defaults to the height of the parent.
  final double? height;

  /// How to inscribe the picture box into the player viewport.
  /// Defaults to [BoxFit.contain].
  final BoxFit fit;

  /// How to align the picture box within the player viewport.
  /// Defaults to [Alignment.center]
  final AlignmentGeometry alignment;

  /// Scale.
  final double scale;

  /// Filter quality.
  final FilterQuality filterQuality;

  // Built-In video controls.
  final bool showControls;

  // Background color.
  final Color? backgroundColor;

  const Video({
    required this.player,
    this.width,
    this.height,
    this.fit = BoxFit.contain,
    this.alignment = Alignment.center,
    this.scale = 1.0,
    this.showControls = true,
    this.backgroundColor = Colors.black,
    this.filterQuality = FilterQuality.low,
    Key? key,
  }) : super(key: key);

  @override
  _VideoStateBase createState() =>
      // ignore: no_logic_in_create_state
      hasTextureSupport ? _VideoStateTexture() : _VideoStateFallback();
}

abstract class _VideoStateBase extends State<Video> {
  int get playerId => widget.player.id;

  @override
  void initState() {
    super.initState();
  }

  @override
  Widget build(BuildContext context) {
    final player = buildPlayer();
    return SizedBox(
        width: widget.width ?? double.infinity,
        height: widget.height ?? double.infinity,
        child: widget.backgroundColor != null
            ? DecoratedBox(
                decoration: BoxDecoration(color: widget.backgroundColor),
                child: player)
            : player);
  }

  Widget buildPlayer();
}

class _VideoStateTexture extends _VideoStateBase {
  StreamSubscription? _videoDimensionsSubscription;
  double _videoWidth = 0;
  double _videoHeight = 0;

  @override
  void initState() {
    super.initState();
    _updateSubscriptions();
  }

  @override
  void didUpdateWidget(Video oldWidget) {
    super.didUpdateWidget(oldWidget);
    _updateSubscriptions();
  }

  void _updateSubscriptions() {
    _videoDimensionsSubscription?.cancel();
    _videoDimensionsSubscription =
        widget.player.videoDimensionsStream.listen((dimensions) {
      setState(() {
        _videoWidth = dimensions.width.toDouble();
        _videoHeight = dimensions.height.toDouble();
      });
    });

    _videoWidth = widget.player.videoDimensions.width.toDouble();
    _videoHeight = widget.player.videoDimensions.height.toDouble();
  }

  @override
  Widget buildPlayer() {
    return FittedBox(
        alignment: widget.alignment,
        clipBehavior: Clip.hardEdge,
        fit: widget.fit,
        child: SizedBox(
            width: _videoWidth,
            height: _videoHeight,
            child: Texture(
              textureId: widget.player.textureId,
              filterQuality: widget.filterQuality,
            )));
  }

  @override
  Future<void> dispose() async {
    _videoDimensionsSubscription?.cancel();
    super.dispose();
  }
}

class _VideoStateFallback extends _VideoStateBase {
  Widget? videoFrameRawImage;
  late StreamSubscription _frameSubscription;

  Future<RawImage> getVideoFrameRawImage(VideoFrame videoFrame) async {
    final image = await videoFrame.toImage();

    return RawImage(
      image: image,
      alignment: widget.alignment,
      fit: widget.fit,
      scale: widget.scale,
      filterQuality: widget.filterQuality,
    );
  }

  @override
  Future<void> dispose() async {
    _frameSubscription.cancel();
    super.dispose();
  }

  @override
  void initState() {
    _frameSubscription =
        widget.player.videoFrameStream.listen((videoFrame) async {
      videoFrameRawImage = await getVideoFrameRawImage(videoFrame);
      if (mounted) setState(() {});
    });
    super.initState();
  }

  @override
  Widget buildPlayer() {
    return videoFrameRawImage != null
        ? SizedBox.expand(child: ClipRect(child: videoFrameRawImage))
        : const SizedBox();
  }
}
