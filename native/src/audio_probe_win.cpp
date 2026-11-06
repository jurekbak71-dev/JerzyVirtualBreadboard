#ifdef _WIN32
#include <windows.h>
#include <miniaudio.h>
#include <iostream>
int wmain(int argc,wchar_t** argv){
 if(argc!=2){std::wcerr<<L"usage: vbal_audio_probe <audio-file>\n";return 2;}
 ma_decoder decoder{};
 auto config=ma_decoder_config_init(ma_format_f32,2,48000);
 const auto result=ma_decoder_init_file_w(argv[1],&config,&decoder);
 if(result!=MA_SUCCESS){std::wcerr<<L"decoder init failed: "<<result<<L"\n";return 3;}
 float frames[512*2]{};
 ma_uint64 got=0;const auto read=ma_decoder_read_pcm_frames(&decoder,frames,512,&got);
 ma_decoder_uninit(&decoder);
 if(read!=MA_SUCCESS||got==0){std::wcerr<<L"decoder read failed\n";return 4;}
 std::wcout<<L"PASS: decoded "<<got<<L" frames\n";return 0;
}
#else
int main(){return 0;}
#endif
