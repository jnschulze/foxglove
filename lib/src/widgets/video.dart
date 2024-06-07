import 'dart:async';

import 'package:flutter/material.dart';
import 'package:foxglove/src/base.dart';
import 'package:foxglove/src/player_platform.dart';

extension on VideoDimensions {
  Size toSize() => Size(width.toDouble(), height.toDouble());
}

class Video extends StatefulWidget {
  final Player player;

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

  /// Background color.
  final Color? backgroundColor;

  /// Whether and how to clip the content.
  final Clip clipBehavior;

  const Video({
    super.key,
    required this.player,
    this.fit = BoxFit.contain,
    this.alignment = Alignment.center,
    this.scale = 1.0,
    this.backgroundColor = Colors.black,
    this.filterQuality = FilterQuality.low,
    this.clipBehavior = Clip.none,
  });

  @override
  State<Video> createState() => _VideoState();
}

class _VideoState extends State<Video> {
  StreamSubscription? _videoDimensionsSubscription;
  Size _videoDimensions = Size.zero;

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
      final videoDimensions = dimensions.toSize();
      if (videoDimensions != _videoDimensions) {
        setState(() => _videoDimensions = videoDimensions);
      }
    });

    _videoDimensions = widget.player.videoDimensions.toSize();
  }

  @override
  Widget build(BuildContext context) {
    final player = buildPlayer();
    return widget.backgroundColor != null
        ? DecoratedBox(
            decoration: BoxDecoration(color: widget.backgroundColor),
            child: player)
        : player;
  }

  Widget buildPlayer() {
    if (_videoDimensions.isEmpty) {
      return const SizedBox();
    }

    return FittedBox(
        alignment: widget.alignment,
        clipBehavior: widget.clipBehavior,
        fit: widget.fit,
        child: SizedBox.fromSize(
            size: _videoDimensions,
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
