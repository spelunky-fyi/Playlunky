#include "debug.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "mod/virtual_filesystem.h"
#include "util/regex.h"
#include "util/on_scope_exit.h"

#include <cassert>
#include <cstdint>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

static constexpr ctll::fixed_string s_CharacterRule{ ".+char_(.*)\\.DDS" };

static VirtualFilesystem* s_Vfs{ nullptr };

struct DetourReadEncrypedFile {
	inline static SigScan::Function<void* (__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xa8\x38\xff\xff\xff"_sig
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		if (s_Vfs) {
			if (auto* file_info = s_Vfs->LoadFile(file_path, alloc_fun)) {
				return file_info;
			}
		}

		return Trampoline(file_path, alloc_fun);
	}
};

struct DetourReadEncrypedFileWithCharacterRandomizer {
	inline static SigScan::Function<void* (__stdcall*)(const char*, void* (*)(size_t size))> Trampoline{
		.Signature = "\x48\x8b\xc4\x48\x89\x58\x18\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x8d\xa8\x38\xff\xff\xff"_sig
	};
	static void* Detour(const char* file_path, void* (*alloc_fun)(size_t size))
	{
		if (s_Vfs) {
			if (ctre::match<s_CharacterRule>(file_path)) {
				const auto all_files = s_Vfs->GetAllFilePaths(file_path);
				if (!all_files.empty()) {
					const auto random_file = all_files[rand() % all_files.size()]; // use something better for randomness?
					const auto random_file_str = random_file.string();
					if (auto* file_info = s_Vfs->LoadSpecificFile(random_file_str.c_str(), alloc_fun)) {
						return file_info;
					}
				}
			}
			else {
				if (auto* file_info = s_Vfs->LoadFile(file_path, alloc_fun)) {
					return file_info;
				}
			}
		}

		return Trampoline(file_path, alloc_fun);
	}
};

enum FMOD_RESULT {
	FMOD_OK,
	FMOD_ERR_BADCOMMAND,
	FMOD_ERR_CHANNEL_ALLOC,
	FMOD_ERR_CHANNEL_STOLEN,
	FMOD_ERR_DMA,
	FMOD_ERR_DSP_CONNECTION,
	FMOD_ERR_DSP_DONTPROCESS,
	FMOD_ERR_DSP_FORMAT,
	FMOD_ERR_DSP_INUSE,
	FMOD_ERR_DSP_NOTFOUND,
	FMOD_ERR_DSP_RESERVED,
	FMOD_ERR_DSP_SILENCE,
	FMOD_ERR_DSP_TYPE,
	FMOD_ERR_FILE_BAD,
	FMOD_ERR_FILE_COULDNOTSEEK,
	FMOD_ERR_FILE_DISKEJECTED,
	FMOD_ERR_FILE_EOF,
	FMOD_ERR_FILE_ENDOFDATA,
	FMOD_ERR_FILE_NOTFOUND,
	FMOD_ERR_FORMAT,
	FMOD_ERR_HEADER_MISMATCH,
	FMOD_ERR_HTTP,
	FMOD_ERR_HTTP_ACCESS,
	FMOD_ERR_HTTP_PROXY_AUTH,
	FMOD_ERR_HTTP_SERVER_ERROR,
	FMOD_ERR_HTTP_TIMEOUT,
	FMOD_ERR_INITIALIZATION,
	FMOD_ERR_INITIALIZED,
	FMOD_ERR_INTERNAL,
	FMOD_ERR_INVALID_FLOAT,
	FMOD_ERR_INVALID_HANDLE,
	FMOD_ERR_INVALID_PARAM,
	FMOD_ERR_INVALID_POSITION,
	FMOD_ERR_INVALID_SPEAKER,
	FMOD_ERR_INVALID_SYNCPOINT,
	FMOD_ERR_INVALID_THREAD,
	FMOD_ERR_INVALID_VECTOR,
	FMOD_ERR_MAXAUDIBLE,
	FMOD_ERR_MEMORY,
	FMOD_ERR_MEMORY_CANTPOINT,
	FMOD_ERR_NEEDS3D,
	FMOD_ERR_NEEDSHARDWARE,
	FMOD_ERR_NET_CONNECT,
	FMOD_ERR_NET_SOCKET_ERROR,
	FMOD_ERR_NET_URL,
	FMOD_ERR_NET_WOULD_BLOCK,
	FMOD_ERR_NOTREADY,
	FMOD_ERR_OUTPUT_ALLOCATED,
	FMOD_ERR_OUTPUT_CREATEBUFFER,
	FMOD_ERR_OUTPUT_DRIVERCALL,
	FMOD_ERR_OUTPUT_FORMAT,
	FMOD_ERR_OUTPUT_INIT,
	FMOD_ERR_OUTPUT_NODRIVERS,
	FMOD_ERR_PLUGIN,
	FMOD_ERR_PLUGIN_MISSING,
	FMOD_ERR_PLUGIN_RESOURCE,
	FMOD_ERR_PLUGIN_VERSION,
	FMOD_ERR_RECORD,
	FMOD_ERR_REVERB_CHANNELGROUP,
	FMOD_ERR_REVERB_INSTANCE,
	FMOD_ERR_SUBSOUNDS,
	FMOD_ERR_SUBSOUND_ALLOCATED,
	FMOD_ERR_SUBSOUND_CANTMOVE,
	FMOD_ERR_TAGNOTFOUND,
	FMOD_ERR_TOOMANYCHANNELS,
	FMOD_ERR_TRUNCATED,
	FMOD_ERR_UNIMPLEMENTED,
	FMOD_ERR_UNINITIALIZED,
	FMOD_ERR_UNSUPPORTED,
	FMOD_ERR_VERSION,
	FMOD_ERR_EVENT_ALREADY_LOADED,
	FMOD_ERR_EVENT_LIVEUPDATE_BUSY,
	FMOD_ERR_EVENT_LIVEUPDATE_MISMATCH,
	FMOD_ERR_EVENT_LIVEUPDATE_TIMEOUT,
	FMOD_ERR_EVENT_NOTFOUND,
	FMOD_ERR_STUDIO_UNINITIALIZED,
	FMOD_ERR_STUDIO_NOT_LOADED,
	FMOD_ERR_INVALID_STRING,
	FMOD_ERR_ALREADY_LOCKED,
	FMOD_ERR_NOT_LOCKED,
	FMOD_ERR_RECORD_DISCONNECTED,
	FMOD_ERR_TOOMANYSAMPLES
};
enum FMOD_MODE : std::uint32_t
{
	DEFAULT = 0x00000000,
	LOOP_OFF = 0x00000001,
	LOOP_NORMAL = 0x00000002,
	LOOP_BIDI = 0x00000004,
	_2D = 0x00000008,
	_3D = 0x00000010,
	CREATESTREAM = 0x00000080,
	CREATESAMPLE = 0x00000100,
	CREATECOMPRESSEDSAMPLE = 0x00000200,
	OPENUSER = 0x00000400,
	OPENMEMORY = 0x00000800,
	OPENMEMORY_POINT = 0x10000000,
	OPENRAW = 0x00001000,
	OPENONLY = 0x00002000,
	ACCURATETIME = 0x00004000,
	MPEGSEARCH = 0x00008000,
	NONBLOCKING = 0x00010000,
	UNIQUE = 0x00020000,
	_3D_HEADRELATIVE = 0x00040000,
	_3D_WORLDRELATIVE = 0x00080000,
	_3D_INVERSEROLLOFF = 0x00100000,
	_3D_LINEARROLLOFF = 0x00200000,
	_3D_LINEARSQUAREROLLOFF = 0x00400000,
	_3D_INVERSETAPEREDROLLOFF = 0x00800000,
	_3D_CUSTOMROLLOFF = 0x04000000,
	_3D_IGNOREGEOMETRY = 0x40000000,
	IGNORETAGS = 0x02000000,
	LOWMEM = 0x08000000,
	VIRTUAL_PLAYFROMSTART = 0x80000000
};
enum class SOUND_FORMAT
{
	NONE,
	PCM8,
	PCM16,
	PCM24,
	PCM32,
	PCMFLOAT,
	BITSTREAM,
	MAX
};
enum class CHANNELORDER : int
{
	DEFAULT,
	WAVEFORMAT,
	PROTOOLS,
	ALLMONO,
	ALLSTEREO,
	ALSA,
	MAX,
};
enum class SOUND_TYPE
{
	UNKNOWN,
	AIFF,
	ASF,
	DLS,
	FLAC,
	FSB,
	IT,
	MIDI,
	MOD,
	MPEG,
	OGGVORBIS,
	PLAYLIST,
	RAW,
	S3M,
	USER,
	WAV,
	XM,
	XMA,
	AUDIOQUEUE,
	AT9,
	VORBIS,
	MEDIA_FOUNDATION,
	MEDIACODEC,
	FADPCM,
	OPUS,
	MAX,
};
enum class TIMEUNIT : std::uint32_t
{
	MS = 0x00000001,
	PCM = 0x00000002,
	PCMBYTES = 0x00000004,
	RAWBYTES = 0x00000008,
	PCMFRACTION = 0x00000010,
	MODORDER = 0x00000100,
	MODROW = 0x00000200,
	MODPATTERN = 0x00000400,
};

struct CREATESOUNDEXINFO
{
	int                      cbsize;
	std::uint32_t            length;
	std::uint32_t            fileoffset;
	int                      numchannels;
	int                      defaultfrequency;
	SOUND_FORMAT             format;
	std::uint32_t            decodebuffersize;
	int                      initialsubsound;
	int                      numsubsounds;
	std::intptr_t            inclusionlist;
	int                      inclusionlistnum;
	void* pcmreadcallback;
	void* pcmsetposcallback;
	void* nonblockcallback;
	std::intptr_t            dlsname;
	std::intptr_t            encryptionkey;
	int                      maxpolyphony;
	std::intptr_t            userdata;
	SOUND_TYPE               suggestedsoundtype;
	void* fileuseropen;
	void* fileuserclose;
	void* fileuserread;
	void* fileuserseek;
	void* fileuserasyncread;
	void* fileuserasynccancel;
	std::intptr_t            fileuserdata;
	int                      filebuffersize;
	CHANNELORDER             channelorder;
	std::intptr_t            initialsoundgroup;
	std::uint32_t            initialseekposition;
	TIMEUNIT                 initialseekpostype;
	int                      ignoresetfilesystem;
	std::uint32_t            audioqueuepolicy;
	std::uint32_t            minmidigranularity;
	int                      nonblockthreadid;
	std::intptr_t            fsbguid;
};

template<class T>
[[nodiscard]] auto ReadFromBuffer(const char*& buffer) {
	const T* ret = reinterpret_cast<const T*>(buffer);
	buffer += sizeof(T);
	return *ret;
};

struct DetourFmodSystemLoadBankMemory {
	inline static SigScan::Function<FMOD_RESULT(__stdcall*)(void*, const char*, int, int, int, void**)> Trampoline{
		.Signature = "\x40\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xec\x68\x01\x00\x00\x48\x8b\x05\x2a\x2a\x2a\x2a\x48\x33\xc4\x48\x89\x84\x24\x50\x01\x00\x00\x4c\x8b\xb4\x24\xd8\x01\x00\x00"_sig,
		.Module = "fmodstudio.dll"
	};
	static FMOD_RESULT Detour(
		[[maybe_unused]] void* fmod_system,
		const char* buffer,
		int length,
		int mode,
		int flags,
		void** bank) {

			{
				const char* current_buffer_pos = buffer;
				[[maybe_unused]] const char* first_data = buffer + 0x283340;
				const char* buffer_end = buffer + length;

				while (const char* fsb_data = (const char*)SigScan::FindPattern("FSB5"_sig, (void*)current_buffer_pos, (void*)buffer_end)) {
					const char* fsb_begin = fsb_data;

					(void)ReadFromBuffer<char[4]>(fsb_data);
					const auto version = ReadFromBuffer<std::uint32_t>(fsb_data);
					if (version == 1) {
						const auto num_samples = ReadFromBuffer<std::uint32_t>(fsb_data);
						const auto sample_headers_size = ReadFromBuffer<std::uint32_t>(fsb_data);
						const auto sample_names_size = ReadFromBuffer<std::uint32_t>(fsb_data);
						const auto sample_datas_size = ReadFromBuffer<std::uint32_t>(fsb_data);
						const auto bank_flags = ReadFromBuffer<std::uint32_t>(fsb_data);
						(void)ReadFromBuffer<char[32]>(fsb_data);

						const auto header_size = fsb_data - fsb_begin;
						[[maybe_unused]] const auto sample_headers_offset = header_size;
						assert(header_size == sample_headers_offset);
						const auto sample_names_offset = header_size + sample_headers_size;
						const auto sample_datas_offset = header_size + sample_headers_size + sample_names_size;

						std::vector<FsbFile::Sample> samples;
						samples.reserve(num_samples);

						for (std::uint32_t i = 0; i < num_samples; i++) {
							auto read_uint64_but_offset_32 = [](const char*& buffer) {
								const auto ret = ReadFromBuffer<std::uint64_t>(buffer);
								buffer -= 4;
								return ret;
							};

							char this_sample_name[256 + 32 + 1024]{};

							auto offset = read_uint64_but_offset_32(fsb_data);
							auto next_header_type = offset & ((1 << 7) - 1);

							(void)ReadFromBuffer<std::uint32_t>(fsb_data);

#define GET_FSB5_OFFSET(X) ((((X) >> (std::uint64_t)7) << (std::uint64_t)5) & (((std::uint64_t)1 << (std::uint64_t)32) - 1))
							offset = GET_FSB5_OFFSET(offset);

							while (next_header_type & 1)
							{
								auto this_header_type = ReadFromBuffer<std::uint32_t>(fsb_data);
								next_header_type = this_header_type & 1;
								const auto header_length = (this_header_type & 0xffffff) >> 1;
								this_header_type >>= 24;

								const char* current_fsb_data = fsb_data;

								switch (this_header_type)
								{
								case 0x2:
									(void)ReadFromBuffer<std::uint8_t>(fsb_data);
									break;
								case 0x4:
									(void)ReadFromBuffer<std::uint32_t>(fsb_data);
									break;
								case 0x6:
									(void)ReadFromBuffer<std::uint64_t>(fsb_data);
									break;
								case 0x8:
								{
									(void)ReadFromBuffer<std::uint8_t>(fsb_data);
									memcpy(this_sample_name, fsb_data, 256);
									fsb_data += 256;
									break;
								}
								case 0xc:  // xma seek
								case 0xe:  // dsp coeff
								case 0x14: // xwma data
								{
									fsb_data += header_length;
									break;
								}
								case 0xa:
									header_length == 1
										? (void)ReadFromBuffer<std::uint8_t>(fsb_data)
										: (void)ReadFromBuffer<std::uint32_t>(fsb_data);
									break; // seen only once
								case 0x10: /*nothing?*/
									break;
								case 0x1a:
									(void)ReadFromBuffer<std::uint32_t>(fsb_data);
									break; // timestamp?
								default:
									break;
								}

								fsb_data = current_fsb_data + header_length;
							}


							// Compute size of data?

							if (sample_names_size != 0) {
								const char* sample_names_data = fsb_begin + sample_names_offset;
								const char* this_sample_name_offset_address = sample_names_data + i * 4;
								const auto name_offset = ReadFromBuffer<std::uint32_t>(this_sample_name_offset_address);
								const char* this_sample_name_address = sample_names_data + name_offset;
								for (size_t j = 0; j < sizeof(this_sample_name); j++) {
									this_sample_name[j] = ReadFromBuffer<char>(this_sample_name_address);
									if (this_sample_name[j] == '\0') {
										break;
									}
								}
							}
							// This should not happen to us?
							else if (!this_sample_name[0]) {
								fmt::format_to(this_sample_name, "{}", i);
							}

							const std::size_t data_offset = sample_datas_offset + offset;
							samples.push_back(FsbFile::Sample{
									.Name = this_sample_name,
									.Offset = (std::uint32_t)data_offset
								});
						}

						std::sort(samples.begin(), samples.end(), [](const FsbFile::Sample& lhs, const FsbFile::Sample& rhs) { return lhs.Offset < rhs.Offset; });

						s_FsbFiles.push_back(FsbFile{
								.Offset{ (std::uint32_t)(fsb_begin - buffer) },
								.Samples{ std::move(samples) }
							});
					}

					current_buffer_pos = fsb_data;
				}
			}

			s_LastBuffer = buffer;
			s_LastLength = length;
			s_LastMode = mode;
			s_LastFlags = flags;

			*bank = nullptr;
			return FMOD_ERR_UNSUPPORTED;
	}

	static FMOD_RESULT DoLastLoad(void* fmod_system, void** bank) {
		if (s_LastBuffer != nullptr) {
			const char* buffer{ nullptr };
			int length{ 0 };
			int mode{ 0 };
			int flags{ 0 };

			std::swap(buffer, s_LastBuffer);
			std::swap(length, s_LastLength);
			std::swap(mode, s_LastMode);
			std::swap(flags, s_LastFlags);

			return Trampoline(fmod_system, buffer, length, mode, flags, bank);
		}

		return FMOD_ERR_UNSUPPORTED;
	}

	static inline const char* s_LastBuffer{ nullptr };
	static inline int s_LastLength{ 0 };
	static inline int s_LastMode{ 0 };
	static inline int s_LastFlags{ 0 };

	struct FsbFile {
		struct Sample {
			std::string Name;
			std::uint32_t Offset;
		};

		std::uint32_t Offset;
		std::vector<Sample> Samples;
	};
	static inline std::vector<FsbFile> s_FsbFiles;
};

struct DetourFmodSystemLoadBankFile {
	inline static SigScan::Function<FMOD_RESULT(__stdcall*)(void*, const char*, int, void**)> Trampoline{
		.Signature = "\x40\x53\x55\x56\x57\x41\x54\x41\x55\x41\x56\x41\x57\x48\x81\xec\x68\x01\x00\x00\x48\x8b\x05\x2a\x2a\x2a\x2a\x48\x33\xc4\x48\x89\x84\x24\x50\x01\x00\x00\x44\x89\x44\x24\x30"_sig,
		.Module = "fmodstudio.dll"
	};
	static FMOD_RESULT Detour(void* fmod_system, const char* file_name, int flags, void** bank) {
		if (const auto file_path = s_Vfs->GetFilePath(file_name)) {
			const std::string file_path_string = file_path.value().string();
			const FMOD_RESULT result = Trampoline(fmod_system, file_path_string.c_str(), flags, bank);
			if (result == FMOD_OK) {
				return FMOD_OK;
			}
		}

		const FMOD_RESULT last_memory_result = DetourFmodSystemLoadBankMemory::DoLastLoad(fmod_system, bank);
		if (last_memory_result == FMOD_OK) {
			return FMOD_OK;
		}

		return Trampoline(fmod_system, file_name, flags, bank);
	}
};

struct DetourFmodSystemCreateSound {
	inline static SigScan::Function<FMOD_RESULT(__stdcall*)(void*, const char*, FMOD_MODE, CREATESOUNDEXINFO*, void**)> Trampoline{
		.ProcName = "?createSound@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z",
		.Module = "fmod.dll"
	};
	static FMOD_RESULT Detour(void* fmod_system, const char* file_name_or_data, FMOD_MODE mode, CREATESOUNDEXINFO* exinfo, void** sound) {
		if (exinfo->numsubsounds != 1) {
			LogInfo("Loading an audio file with exinfo->numsubsounds != 0, falling back to loading original file...");
			return Trampoline(fmod_system, file_name_or_data, mode, exinfo, sound);
		}
		
		for (const auto& fsb_file : DetourFmodSystemLoadBankMemory::s_FsbFiles) {
			if (fsb_file.Offset == exinfo->fileoffset) {
				const auto sample_index = exinfo->inclusionlist >> 1;
				const std::string file_path = fmt::format("Mods/Extracted/soundbank/wav/{}.wav", fsb_file.Samples[sample_index].Name);

				if (std::filesystem::exists(file_path)) {

					// TODO:
					//	- Cache SOUND** with s_Samples (s_Samples as user data to cancel destroySound until unloading soundbank)
					//	- Move fmod crap to its own file
					//	- Fix loading ogg files
					//	- Don't leak `sound` on failing to load the sub_sound
					//	- Cleanup

					char empty_wav[]{
						"\x52\x49\x46\x46\x25\x00\x00\x00\x57\x41\x56\x45\x66\x6D\x74\x20"
						"\x10\x00\x00\x00\x01\x00\x01\x00\x44\xAC\x00\x00\x88\x58\x01\x00"
						"\x02\x00\x10\x00\x64\x61\x74\x61\x74\x00\x00\x00\x00"
					};

					const FMOD_MODE loose_mode = (FMOD_MODE)(mode | FMOD_MODE::OPENMEMORY_POINT);

					CREATESOUNDEXINFO loose_exinfo{
						.cbsize = sizeof(loose_exinfo),
						.length = sizeof(empty_wav),
						.fileoffset = 0,
						.numsubsounds = 1,
						.inclusionlist = 1,
						.fsbguid = exinfo->fsbguid
					};
					auto ret = Trampoline(fmod_system, empty_wav, loose_mode, &loose_exinfo, sound);

					if (ret == FMOD_OK) {
						auto get_sub_sound = [](void* sound, int sub_sound_index) -> void** {
							const auto* num_sub_sounds = reinterpret_cast<int*>(sound) + 0x2c;
							if (*num_sub_sounds <= sub_sound_index) {
								return nullptr;
							}
							const auto* sub_sounds_array = reinterpret_cast<void***>(sound) + 0x14;
							return (*sub_sounds_array) + sub_sound_index;
						};

						if (void** sub_sound = get_sub_sound(*sound, 0))
						{
							static auto read_file_data = [](const std::string& file_path) {
								std::vector<char> file_data_tmp;
								FILE* file{ nullptr };
								auto error = fopen_s(&file, file_path.c_str(), "rb");
								if (error == 0 && file != nullptr) {
									auto close_file = OnScopeExit{ [file]() { fclose(file); } };

									fseek(file, 0, SEEK_END);
									const std::size_t file_size = ftell(file);
									fseek(file, 0, SEEK_SET);

									file_data_tmp.resize(file_size);
									const auto size_read = fread(file_data_tmp.data(), 1, file_size, file);
									if (size_read != file_size) {
										LogInfo("Could not read file {}, this will either crash or cause glitches...", file_path);
									}
								}
								return file_data_tmp;
							};
							std::vector<char> file_data = read_file_data(file_path);

							const FMOD_MODE loose_sub_sound_mode = (FMOD_MODE)((mode | FMOD_MODE::OPENMEMORY) & ~FMOD_MODE::OPENMEMORY_POINT);

							loose_exinfo.length = (int)file_data.size();
							loose_exinfo.numsubsounds = 0;
							ret = Trampoline(fmod_system, file_data.data(), loose_sub_sound_mode, &loose_exinfo, sub_sound);

							return ret;
						}

						// Leaking `sound` if we get here
					}
				}
			}
		}

		return Trampoline(fmod_system, file_name_or_data, mode, exinfo, sound);
	}
};

struct DetourFmodSystemCreateStream {
	inline static SigScan::Function<FMOD_RESULT(__stdcall*)(void*, const char*, FMOD_MODE, CREATESOUNDEXINFO*, void**)> Trampoline{
		.ProcName = "?createStream@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z",
		.Module = "fmod.dll"
	};
	static FMOD_RESULT Detour(void* fmod_system, const char* file_name_or_data, FMOD_MODE mode, CREATESOUNDEXINFO* exinfo, void** sound) {
		LogInfo("Loading an audio stream which has not been patched, falling back to loading original stream...");
		return Trampoline(fmod_system, file_name_or_data, mode, exinfo, sound);
	}
};

std::vector<DetourEntry> GetFileIODetours() {
	const bool random_char_select = []() {
		return INIReader("playlunky.ini").GetBoolean("settings", "random_character_select", false);
	}();
	if (random_char_select) {
		srand(static_cast<unsigned int>(time(nullptr))); // use something better for randomness?
		return {
			DetourHelper<DetourReadEncrypedFileWithCharacterRandomizer>::GetDetourEntry("ReadEncrypedFile"),
			DetourHelper<DetourFmodSystemLoadBankMemory>::GetDetourEntry("FMOD::System::loadBankMemory"),
			DetourHelper<DetourFmodSystemLoadBankFile>::GetDetourEntry("FMOD::System::loadBankFile"),
			DetourHelper<DetourFmodSystemCreateSound>::GetDetourEntry("FMOD::System::createSound"),
			DetourHelper<DetourFmodSystemCreateStream>::GetDetourEntry("FMOD::System::createStream")
		};
	}
	else {
		return {
			DetourHelper<DetourReadEncrypedFile>::GetDetourEntry("ReadEncrypedFile"),
			DetourHelper<DetourFmodSystemLoadBankMemory>::GetDetourEntry("FMOD::System::loadBankMemory"),
			DetourHelper<DetourFmodSystemLoadBankFile>::GetDetourEntry("FMOD::System::loadBankFile"),
			DetourHelper<DetourFmodSystemCreateSound>::GetDetourEntry("FMOD::System::createSound"),
			DetourHelper<DetourFmodSystemCreateStream>::GetDetourEntry("FMOD::System::createStream")
		};
	}
}

void SetVfs(VirtualFilesystem* vfs) {
	s_Vfs = vfs;
}
