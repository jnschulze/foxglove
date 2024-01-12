#pragma once

#include <mutex>

#include "events.h"
#include "player.h"
#include "vlc/vlc_environment.h"

namespace foxglove {

class VlcPlayer : public Player {
 public:
  VlcPlayer(std::shared_ptr<VlcEnvironment> environment);
  ~VlcPlayer() override;

  void SetEventDelegate(
      std::unique_ptr<PlayerEventDelegate> event_delegate) override;
  PlayerEventDelegate* event_delegate() const override;

  // |VideoOutputFactory|
  std::unique_ptr<VideoOutput> CreatePixelBufferOutput(
      std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
      PixelFormat pixel_format) const override;

#ifdef _WIN32
  // |VideoOutputFactory|
  std::unique_ptr<VideoOutput> CreateD3D11Output(
      std::unique_ptr<D3D11OutputDelegate> output_delegate,
      winrt::com_ptr<IDXGIAdapter> adapter = nullptr) const override;
#endif

  Status<ErrorDetails> SetVideoOutput(
      std::unique_ptr<VideoOutput> output) override;
  VideoOutput* GetVideoOutput() const override;

  bool Open(std::unique_ptr<Media> media) override;
  bool Play() override;
  void Pause() override;
  bool Stop() override;
  void SeekPosition(double position) override;
  void SeekTime(int64_t time) override;
  void SetRate(float rate);
  void SetLoopMode(LoopMode loop_mode) override;
  void SetVolume(double volume) override;
  void SetMute(bool is_muted) override;
  int64_t duration() override;

 private:
  class Impl;
  std::shared_ptr<Impl> impl_;
};

}  // namespace foxglove
