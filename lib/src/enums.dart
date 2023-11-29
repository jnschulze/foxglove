enum MediaType {
  file,
  network,
  asset,
  directShow,
}

enum LoopMode { off, loop }

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
