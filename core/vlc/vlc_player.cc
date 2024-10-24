#include "vlc/vlc_player.h"

#include "base/logging.h"
#include "vlc/vlc_d3d11_output.h"
#include "vlc/vlc_pixel_buffer_output.h"
#include "vlc_player_impl.h"

namespace foxglove {

VlcPlayer::VlcPlayer(std::shared_ptr<VlcEnvironment> environment) {
  impl_ = std::make_shared<Impl>(std::move(environment), id());
}

VlcPlayer::~VlcPlayer() { LOG(TRACE) << "Destructing VlcPlayer" << std::endl; }

std::unique_ptr<VlcPlayer::VideoOutputType> VlcPlayer::CreatePixelBufferOutput(
    std::unique_ptr<PixelBufferOutputDelegate> output_delegate,
    PixelFormat pixel_format) const {
  return std::make_unique<VlcPixelBufferOutput>(std::move(output_delegate),
                                                pixel_format);
}

#ifdef _WIN32
std::unique_ptr<VlcPlayer::VideoOutputType> VlcPlayer::CreateD3D11Output(
    std::unique_ptr<D3D11OutputDelegate> output_delegate,
    winrt::com_ptr<IDXGIAdapter> adapter) const {
  return std::make_unique<VlcD3D11Output>(std::move(output_delegate), adapter);
}
#endif

Status<ErrorDetails> VlcPlayer::SetVideoOutput(
    std::unique_ptr<VlcPlayer::VideoOutputType> video_output) {
  assert(impl_);
  assert(video_output);
  return impl_->SetVideoOutput(std::move(video_output));
}

VlcPlayer::VideoOutputType* VlcPlayer::GetVideoOutput() const {
  assert(impl_);
  return impl_->GetVideoOutput();
}

void VlcPlayer::SetEventDelegate(
    std::unique_ptr<PlayerEventDelegate> event_delegate) {
  assert(impl_);
  impl_->SetEventDelegate(std::move(event_delegate));
}

PlayerEventDelegate* VlcPlayer::event_delegate() const {
  assert(impl_);
  return impl_->event_delegate();
}

bool VlcPlayer::Open(std::unique_ptr<Media> media) {
  assert(impl_);
  return impl_->Open(std::move(media));
}

bool VlcPlayer::Play() {
  assert(impl_);
  return impl_->Play();
}

bool VlcPlayer::Stop() {
  assert(impl_);
  return impl_->Stop();
}

void VlcPlayer::Pause() {
  assert(impl_);
  impl_->Pause();
}

void VlcPlayer::SeekPosition(double position) {
  assert(impl_);
  impl_->SeekPosition(position);
}

void VlcPlayer::SeekTime(int64_t time) {
  assert(impl_);
  impl_->SeekTime(time);
}

void VlcPlayer::SetRate(float rate) {
  assert(impl_);
  impl_->SetRate(rate);
}

void VlcPlayer::SetLoopMode(LoopMode loop_mode) {
  assert(impl_);
  impl_->SetLoopMode(loop_mode);
}

void VlcPlayer::SetVolume(double volume) {
  assert(impl_);
  impl_->SetVolume(volume);
}

void VlcPlayer::SetMute(bool flag) {
  assert(impl_);
  impl_->SetMute(flag);
}

void VlcPlayer::SetPositionReportingEnabled(bool is_enabled) {
  assert(impl_);
  impl_->SetPositionReportingEnabled(is_enabled);
}

int64_t VlcPlayer::duration() {
  assert(impl_);
  return impl_->duration();
}

}  // namespace foxglove
