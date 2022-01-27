enum MediaSourceType {
  media,
  playlist,
}

enum MediaType {
  file,
  network,
  asset,
  directShow,
}

enum PlaylistMode { single, loop, repeat }

enum PlaybackState {
  none,
  opening,
  buffering,
  playing,
  paused,
  stopped,
  ended,
  error
}
