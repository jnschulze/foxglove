import 'dart:io';

import 'package:foxglove/src/enums.dart';
import 'package:foxglove/src/media/media_source.dart';
import 'package:path/path.dart' as path;

class Media implements MediaSource {
  final MediaType mediaType;
  final String resource;

  const Media._({required this.mediaType, required this.resource});

  factory Media.file(File file) {
    return Media._(mediaType: MediaType.file, resource: file.path);
  }

  factory Media.network(dynamic url) {
    final resource = (url is Uri) ? url.toString() : url;
    return Media._(mediaType: MediaType.network, resource: resource);
  }

  factory Media.directShow(
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

  factory Media.asset(String asset) {
    if (Platform.isWindows || Platform.isLinux) {
      final assetPath = path.join(path.dirname(Platform.resolvedExecutable),
          'data', 'flutter_assets', asset);
      final url = Uri.file(assetPath, windows: Platform.isWindows);
      return Media._(mediaType: MediaType.asset, resource: url.toString());
    }
    throw UnimplementedError('The platform is not supported');
  }

  factory Media.fromJson(Map<dynamic, dynamic> json) {
    final type = json['type'] as String;
    final url = json['resource'] as String;
    switch (type) {
      case 'file':
        return Media.file(File(url));
      case 'network':
        return Media.network(url);
      case 'asset':
        return Media.asset(url);
      case 'directShow':
        return Media.directShow(rawUrl: url);
      default:
        throw 'Unsupported media type: $type';
    }
  }

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
  int get hashCode => Object.hash(mediaType, resource);

  @override
  bool operator ==(Object other) {
    if (identical(this, other)) return true;
    return other is Media &&
        other.mediaType == mediaType &&
        other.resource == resource;
  }

  @override
  String toString() => '[$mediaType]$resource';

  @override
  Map<String, dynamic> toJson() {
    return {'type': mediaType.name, 'resource': resource};
  }
}
