import 'dart:io';

import 'package:foxglove/src/enums/media_source_type.dart';
import 'package:foxglove/src/enums/media_type.dart';
import 'package:foxglove/src/media_source/media_source.dart';
import 'package:path/path.dart' as path;

/// A media object to open inside a [Player].
///
/// Pass `true` to [parse] for retrieving the metadata of the [Media].
/// [timeout] sets the time-limit for retriveing metadata.
/// [Media.metas] can be then, accessed to get the retrived metadata as `Map<String, String>`.
///
/// * A [Media] from a [File].
///
/// ```dart
/// Media media = await Media.file(new File('C:/music.ogg'));
/// ```
///
/// * A [Media] from a [Uri].
///
/// ```dart
/// Media media = await Media.network('http://alexmercerind.github.io/music.mp3');
/// ```
///
///
class Media implements MediaSource {
  @override
  MediaSourceType get mediaSourceType => MediaSourceType.media;
  final MediaType mediaType;
  final String resource;
  final Map<String, String> metas;

  const Media._(
      {required this.mediaType, required this.resource, this.metas = const {}});

  /// Makes [Media] object from a [File].
  static Media file(File file,
      {bool parse = false,
      Map<String, dynamic>? extras,
      Duration timeout = const Duration(seconds: 10)}) {
    final media = Media._(mediaType: MediaType.file, resource: file.path);

    if (parse) {
      media.parse(timeout);
    }
    return media;
  }

  /// Makes [Media] object from url.
  static Media network(dynamic url,
      {bool parse = false,
      Map<String, dynamic>? extras,
      Duration timeout = const Duration(seconds: 10)}) {
    final resource = (url is Uri) ? url.toString() : url;
    final Media media =
        Media._(mediaType: MediaType.network, resource: resource);

    if (parse) {
      media.parse(timeout);
    }
    return media;
  }

  /// Makes [Media] object from direct show.
  static Media directShow(
      {String? rawUrl,
      Map<String, dynamic>? args,
      String? vdev,
      String? adev,
      int? liveCaching}) {
    final resourceUrl = rawUrl ??
        _buildDirectShowUrl(args ??
            {
              'dshow-vdev': vdev,
              'dshow-adev': adev,
              'live-caching': liveCaching
            });

    return Media._(mediaType: MediaType.directShow, resource: resourceUrl);
  }

  /// Makes [Media] object from assets.
  ///
  /// **WARNING**
  ///
  /// This method only works for Flutter.
  /// Might result in an exception on Dart CLI.
  ///
  static Media asset(String asset) {
    if (Platform.isWindows || Platform.isLinux) {
      final assetPath = path.join(path.dirname(Platform.resolvedExecutable),
          'data', 'flutter_assets', asset);
      final url = Uri.file(assetPath, windows: Platform.isWindows);
      return Media._(mediaType: MediaType.asset, resource: url.toString());
    }

    // TODO: Add macOS asset path support.
    throw UnimplementedError('The platform is not supported');
  }

  /// Parses the [Media] to retrieve [Media.metas].
  void parse(Duration timeout) {}

  static String _buildDirectShowUrl(Map<String, dynamic> args) {
    return args.entries.fold(
        'dshow://',
        (prev, pair) =>
            prev +
            (pair.value != null
                ? ' :${pair.key.toLowerCase()}=${pair.value}'
                : ''));
  }

  @override
  int get hashCode => mediaType.hashCode ^ resource.hashCode;

  @override
  bool operator ==(Object other) =>
      other is Media &&
      other.mediaType == mediaType &&
      other.resource == resource;

  @override
  String toString() => '[$mediaType]$resource';
}
