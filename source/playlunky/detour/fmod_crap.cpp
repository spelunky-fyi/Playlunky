#include "fmod_crap.h"

#include "detour_entry.h"
#include "detour_helper.h"
#include "sigscan.h"
#include "log.h"
#include "mod/decode_audio_file.h"
#include "mod/virtual_filesystem.h"
#include "util/on_scope_exit.h"

#include <cassert>
#include <cstdint>

#pragma warning(push)
#pragma warning(disable : 4996)
#pragma warning(disable : 4244)
#include <INIReader.h>
#pragma warning(pop)

static VirtualFilesystem* s_FmodVfs{ nullptr };

namespace FMOD {
	enum FMOD_RESULT {
		OK,
		ERR_BADCOMMAND,
		ERR_CHANNEL_ALLOC,
		ERR_CHANNEL_STOLEN,
		ERR_DMA,
		ERR_DSP_CONNECTION,
		ERR_DSP_DONTPROCESS,
		ERR_DSP_FORMAT,
		ERR_DSP_INUSE,
		ERR_DSP_NOTFOUND,
		ERR_DSP_RESERVED,
		ERR_DSP_SILENCE,
		ERR_DSP_TYPE,
		ERR_FILE_BAD,
		ERR_FILE_COULDNOTSEEK,
		ERR_FILE_DISKEJECTED,
		ERR_FILE_EOF,
		ERR_FILE_ENDOFDATA,
		ERR_FILE_NOTFOUND,
		ERR_FORMAT,
		ERR_HEADER_MISMATCH,
		ERR_HTTP,
		ERR_HTTP_ACCESS,
		ERR_HTTP_PROXY_AUTH,
		ERR_HTTP_SERVER_ERROR,
		ERR_HTTP_TIMEOUT,
		ERR_INITIALIZATION,
		ERR_INITIALIZED,
		ERR_INTERNAL,
		ERR_INVALID_FLOAT,
		ERR_INVALID_HANDLE,
		ERR_INVALID_PARAM,
		ERR_INVALID_POSITION,
		ERR_INVALID_SPEAKER,
		ERR_INVALID_SYNCPOINT,
		ERR_INVALID_THREAD,
		ERR_INVALID_VECTOR,
		ERR_MAXAUDIBLE,
		ERR_MEMORY,
		ERR_MEMORY_CANTPOINT,
		ERR_NEEDS3D,
		ERR_NEEDSHARDWARE,
		ERR_NET_CONNECT,
		ERR_NET_SOCKET_ERROR,
		ERR_NET_URL,
		ERR_NET_WOULD_BLOCK,
		ERR_NOTREADY,
		ERR_OUTPUT_ALLOCATED,
		ERR_OUTPUT_CREATEBUFFER,
		ERR_OUTPUT_DRIVERCALL,
		ERR_OUTPUT_FORMAT,
		ERR_OUTPUT_INIT,
		ERR_OUTPUT_NODRIVERS,
		ERR_PLUGIN,
		ERR_PLUGIN_MISSING,
		ERR_PLUGIN_RESOURCE,
		ERR_PLUGIN_VERSION,
		ERR_RECORD,
		ERR_REVERB_CHANNELGROUP,
		ERR_REVERB_INSTANCE,
		ERR_SUBSOUNDS,
		ERR_SUBSOUND_ALLOCATED,
		ERR_SUBSOUND_CANTMOVE,
		ERR_TAGNOTFOUND,
		ERR_TOOMANYCHANNELS,
		ERR_TRUNCATED,
		ERR_UNIMPLEMENTED,
		ERR_UNINITIALIZED,
		ERR_UNSUPPORTED,
		ERR_VERSION,
		ERR_EVENT_ALREADY_LOADED,
		ERR_EVENT_LIVEUPDATE_BUSY,
		ERR_EVENT_LIVEUPDATE_MISMATCH,
		ERR_EVENT_LIVEUPDATE_TIMEOUT,
		ERR_EVENT_NOTFOUND,
		ERR_STUDIO_UNINITIALIZED,
		ERR_STUDIO_NOT_LOADED,
		ERR_INVALID_STRING,
		ERR_ALREADY_LOCKED,
		ERR_NOT_LOCKED,
		ERR_RECORD_DISCONNECTED,
		ERR_TOOMANYSAMPLES
	};
	enum FMOD_MODE : std::uint32_t
	{
		MODE_DEFAULT = 0x00000000,
		MODE_LOOP_OFF = 0x00000001,
		MODE_LOOP_NORMAL = 0x00000002,
		MODE_LOOP_BIDI = 0x00000004,
		MODE_2D = 0x00000008,
		MODE_3D = 0x00000010,
		MODE_CREATESTREAM = 0x00000080,
		MODE_CREATESAMPLE = 0x00000100,
		MODE_CREATECOMPRESSEDSAMPLE = 0x00000200,
		MODE_OPENUSER = 0x00000400,
		MODE_OPENMEMORY = 0x00000800,
		MODE_OPENMEMORY_POINT = 0x10000000,
		MODE_OPENRAW = 0x00001000,
		MODE_OPENONLY = 0x00002000,
		MODE_ACCURATETIME = 0x00004000,
		MODE_MPEGSEARCH = 0x00008000,
		MODE_NONBLOCKING = 0x00010000,
		MODE_UNIQUE = 0x00020000,
		MODE_3D_HEADRELATIVE = 0x00040000,
		MODE_3D_WORLDRELATIVE = 0x00080000,
		MODE_3D_INVERSEROLLOFF = 0x00100000,
		MODE_3D_LINEARROLLOFF = 0x00200000,
		MODE_3D_LINEARSQUAREROLLOFF = 0x00400000,
		MODE_3D_INVERSETAPEREDROLLOFF = 0x00800000,
		MODE_3D_CUSTOMROLLOFF = 0x04000000,
		MODE_3D_IGNOREGEOMETRY = 0x40000000,
		MODE_IGNORETAGS = 0x02000000,
		MODE_LOWMEM = 0x08000000,
		MODE_VIRTUAL_PLAYFROMSTART = 0x80000000
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
		void*                    pcmreadcallback;
		void*                    pcmsetposcallback;
		void*                    nonblockcallback;
		std::intptr_t            dlsname;
		std::intptr_t            encryptionkey;
		int                      maxpolyphony;
		std::intptr_t            userdata;
		SOUND_TYPE               suggestedsoundtype;
		void*                    fileuseropen;
		void*                    fileuserclose;
		void*                    fileuserread;
		void*                    fileuserseek;
		void*                    fileuserasyncread;
		void*                    fileuserasynccancel;
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

	using System = void;
	using Bank = void;
	using Sound = void;
}

template<class T>
[[nodiscard]] inline auto ReadFromBuffer(const char*& buffer) {
	const T* ret = reinterpret_cast<const T*>(buffer);
	buffer += sizeof(T);
	return *ret;
};

inline FMOD::FMOD_RESULT CreateSound(FMOD::System* fmod_system, const char* filename_or_data, FMOD::FMOD_MODE mode, FMOD::CREATESOUNDEXINFO* exinfo, FMOD::Sound** sound);
inline FMOD::FMOD_RESULT ReleaseSound(FMOD::Sound* sound);

struct DetourFmodSystemLoadBankMemory {
	inline static SigScan::Function<FMOD::FMOD_RESULT(__stdcall*)(FMOD::System*, const char*, int, int, int, FMOD::Bank**)> Trampoline{
		.ProcName = "?loadBankMemory@System@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDHW4FMOD_STUDIO_LOAD_MEMORY_MODE@@IPEAPEAVBank@23@@Z",
		.Module = "fmodstudio.dll"
	};
	static FMOD::FMOD_RESULT Detour(
		[[maybe_unused]] FMOD::System* fmod_system,
		const char* buffer,
		int length,
		int mode,
		int flags,
		FMOD::Bank** bank) {

		s_LastBuffer = buffer;
		s_LastLength = length;
		s_LastMode = mode;
		s_LastFlags = flags;

		*bank = nullptr;
		return FMOD::ERR_UNSUPPORTED;
	}

	static void ParseSoundbankMemory() {
		if (!s_EnableLooseFiles)
			return;

		LogInfo("Parsing bank file to find all samples within...");

		const auto buffer = s_LastBuffer;
		const auto length = s_LastLength;

		std::size_t num_samples{ 0 };
		if (buffer != nullptr && length != 0) {
			const char* current_buffer_pos = buffer;
			[[maybe_unused]] const char* first_data = buffer + 0x283340;
			const char* buffer_end = buffer + length;

			while (const char* fsb_data = (const char*)SigScan::FindPattern("FSB5"_sig, (void*)current_buffer_pos, (void*)buffer_end)) {
				const char* fsb_begin = fsb_data;

				(void)ReadFromBuffer<char[4]>(fsb_data);
				const auto version = ReadFromBuffer<std::uint32_t>(fsb_data);
				if (version == 1) {
					const auto num_samples_in_fsb = ReadFromBuffer<std::uint32_t>(fsb_data);
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
					samples.reserve(num_samples_in_fsb);

					for (std::uint32_t i = 0; i < num_samples_in_fsb; i++) {
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
#undef GET_FSB5_OFFSET

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
						num_samples++;
					}

					std::sort(samples.begin(), samples.end(), [](const FsbFile::Sample& lhs, const FsbFile::Sample& rhs) { return lhs.Offset < rhs.Offset; });

					s_FsbFiles.push_back(FsbFile{
							.Offset{ (std::uint32_t)(fsb_begin - buffer) },
							.Samples{ std::move(samples) },
							.Bank{ nullptr }
						});
				}

				current_buffer_pos = fsb_data;
			}
		}

		LogInfo("Found {} samples...", num_samples);
	}

	static void PreloadModdedSampleData([[maybe_unused]] FMOD::System* fmod_system, FMOD::Bank* bank) {
		if (!s_EnableLooseFiles)
			return;

		LogInfo("Preloading any modded samples...");

		std::size_t num_samples{ 0 };
		for (FsbFile& fsb_file : s_FsbFiles) {
			if (fsb_file.Bank == nullptr) {
				fsb_file.Bank = bank;

				for (FsbFile::Sample& sample : fsb_file.Samples) {
					const auto modded_sample = s_FmodVfs->GetFilePath(fmt::format("soundbank/wav/{}.wav", sample.Name))
						.value_or(s_FmodVfs->GetFilePath(fmt::format("soundbank/ogg/{}.ogg", sample.Name))
							.value_or("invalid.err"));
					if (std::filesystem::exists(modded_sample)) {

						sample.Buffer = DecodeAudioFile(modded_sample);

						// BOOKMARK-POSSIBLE-OPT
						// Possible optimization? Preload sounds here?
						/*
						static char empty_wav[]{
							"\x52\x49\x46\x46\x25\x00\x00\x00\x57\x41\x56\x45\x66\x6D\x74\x20"
							"\x10\x00\x00\x00\x01\x00\x01\x00\x44\xAC\x00\x00\x88\x58\x01\x00"
							"\x02\x00\x10\x00\x64\x61\x74\x61\x74\x00\x00\x00\x00"
						};

						// Need to specify OPENMEMORY_POINT in case the .bank file is loose
						const FMOD::FMOD_MODE loose_mode = (FMOD::FMOD_MODE)(mode | FMOD::MODE_OPENMEMORY_POINT);

						FMOD::CREATESOUNDEXINFO loose_exinfo{
							.cbsize = sizeof(loose_exinfo),
							.length = sizeof(empty_wav),
							.fileoffset = 0,
							.numsubsounds = 1,
							.inclusionlist = 1
						};
						const auto create_empty_sound_res = Trampoline(fmod_system, empty_wav, loose_mode, &loose_exinfo, sound);

						if (create_empty_sound_res == FMOD::OK) {
							auto get_sub_sound = [](FMOD::Sound* sound, int sub_sound_index) ->FMOD::Sound** {
								const auto* num_sub_sounds = reinterpret_cast<int*>(sound) + 0x2c;
								if (*num_sub_sounds <= sub_sound_index) {
									return nullptr;
								}
								const auto* sub_sounds_array = reinterpret_cast<FMOD::Sound***>(sound) + 0x14;
								return (*sub_sounds_array) + sub_sound_index;
							};

							if (FMOD::Sound** sub_sound = get_sub_sound(*sound, 0))
							{
								if (*sub_sound != nullptr) {
									ReleaseSound(*sub_sound);
								}

								const FMOD::FMOD_MODE loose_sub_sound_mode = (FMOD::FMOD_MODE)(mode | FMOD::MODE_OPENMEMORY_POINT | FMOD::MODE_OPENRAW);

								FMOD::CREATESOUNDEXINFO loose_subsound_exinfo{
									.cbsize = sizeof(loose_subsound_exinfo),
									.length = (std::uint32_t)sample.Buffer.DataSize,
									.numchannels = sample.Buffer.NumChannels,
									.defaultfrequency = sample.Buffer.Frequency,
									.format = [](std::int32_t bits_per_sample) {
										switch (bits_per_sample) {
										default:
										case 8:
											return FMOD::SOUND_FORMAT::PCM8;
										case 16:
											return FMOD::SOUND_FORMAT::PCM16;
										case 24:
											return FMOD::SOUND_FORMAT::PCM24;
										case 32:
											return FMOD::SOUND_FORMAT::PCM32;
										}
									}(sample.Buffer.BitsPerSample),
									.numsubsounds = 0
								};

								const auto create_sub_sound_res = Trampoline(fmod_system, (const char*)sample.Buffer.Data.get(), loose_sub_sound_mode, &loose_subsound_exinfo, sub_sound);
								if (create_sub_sound_res == FMOD::OK) {
									return FMOD::OK;
								}

								LogInfo("Failed loading loose audio file {}, falling back to loading from soundbank...", sample.Name);
								[[maybe_unused]] const auto destroy_leaking_sound_res = ReleaseSound(*sound);
								assert(destroy_leaking_sound_res == FMOD::OK);
							}
						}
						*/
					}
				}
			}
		}
		
		LogInfo("Preloaded {} modded samples...", num_samples);
	}

	static FMOD::FMOD_RESULT DoLastLoad(FMOD::System* fmod_system, FMOD::Sound** bank) {
		if (s_LastBuffer != nullptr) {
			const char* buffer{ nullptr };
			int length{ 0 };
			int mode{ 0 };
			int flags{ 0 };

			std::swap(buffer, s_LastBuffer);
			std::swap(length, s_LastLength);
			std::swap(mode, s_LastMode);
			std::swap(flags, s_LastFlags);

			auto last_load_res = Trampoline(fmod_system, buffer, length, mode, flags, bank);
			PreloadModdedSampleData(fmod_system , *bank);
			return last_load_res;
		}

		return FMOD::ERR_UNSUPPORTED;
	}

	static inline const char* s_LastBuffer{ nullptr };
	static inline int s_LastLength{ 0 };
	static inline int s_LastMode{ 0 };
	static inline int s_LastFlags{ 0 };

	struct FsbFile {
		struct Sample {
			std::string Name;
			std::uint32_t Offset;
			FMOD::Sound* Sound;
			DecodedAudioBuffer Buffer;

			std::unique_ptr<std::byte[]> Data;
			std::uint32_t DataSize;
		};

		std::uint32_t Offset;
		std::vector<Sample> Samples;
		FMOD::Bank* Bank;
	};
	static inline std::vector<FsbFile> s_FsbFiles;
	inline static bool s_EnableLooseFiles{ true };
};

struct DetourFmodSystemLoadBankFile {
	inline static SigScan::Function<FMOD::FMOD_RESULT(__stdcall*)(FMOD::System*, const char*, int, FMOD::Bank**)> Trampoline{
		.ProcName = "?loadBankFile@System@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAPEAVBank@23@@Z",
		.Module = "fmodstudio.dll"
	};
	static FMOD::FMOD_RESULT Detour(FMOD::System* fmod_system, const char* file_name, int flags, FMOD::Bank** bank) {
		LogInfo("Loading bank file {} into FMOD...", file_name);

		DetourFmodSystemLoadBankMemory::ParseSoundbankMemory();

		if (const auto file_path = s_FmodVfs->GetFilePath(file_name)) {
			const std::string file_path_string = file_path.value().string();
			const FMOD::FMOD_RESULT result = Trampoline(fmod_system, file_path_string.c_str(), flags, bank);
			if (result == FMOD::OK) {
				DetourFmodSystemLoadBankMemory::PreloadModdedSampleData(fmod_system, *bank);
				return FMOD::OK;
			}
		}

		const FMOD::FMOD_RESULT last_memory_result = DetourFmodSystemLoadBankMemory::DoLastLoad(fmod_system, bank);
		if (last_memory_result == FMOD::OK) {
			return FMOD::OK;
		}

		return Trampoline(fmod_system, file_name, flags, bank);
	}
};

struct DetourFmodSystemUnloadBank {
	inline static SigScan::Function<FMOD::FMOD_RESULT(__stdcall*)(FMOD::Bank*)> Trampoline{
		.ProcName = "?unload@Bank@Studio@FMOD@@QEAA?AW4FMOD_RESULT@@XZ",
		.Module = "fmodstudio.dll"
	};
	static FMOD::FMOD_RESULT Detour(FMOD::Bank* bank) {
		// BOOKMARK-POSSIBLE-OPT
		//for (auto it = DetourFmodSystemLoadBankMemory::s_FsbFiles.begin(); it != DetourFmodSystemLoadBankMemory::s_FsbFiles.end(); ++it) {
		//	if (it->Bank == bank) {
		//		for (const auto& sample : it->Samples) {
		//			if (sample.Sound != nullptr) {
		//				ReleaseSound(sample.Sound);
		//			}
		//		}
		//		DetourFmodSystemLoadBankMemory::s_FsbFiles.erase(it);
		//		break;
		//	}
		//}

		return Trampoline(bank);
	}
};

struct DetourFmodSystemCreateSound {
	inline static SigScan::Function<FMOD::FMOD_RESULT(__stdcall*)(FMOD::System*, const char*, FMOD::FMOD_MODE, FMOD::CREATESOUNDEXINFO*, FMOD::Sound**)> Trampoline{
		.ProcName = "?createSound@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z",
		.Module = "fmod.dll"
	};
	static FMOD::FMOD_RESULT Detour(FMOD::System* fmod_system, const char* file_name_or_data, FMOD::FMOD_MODE mode, FMOD::CREATESOUNDEXINFO* exinfo, FMOD::Sound** sound) {
		if (exinfo->numsubsounds != 1) {
			LogInfo("Loading an audio file with exinfo->numsubsounds != 0, falling back to loading original file...");
			return Trampoline(fmod_system, file_name_or_data, mode, exinfo, sound);
		}

		for (const auto& fsb_file : DetourFmodSystemLoadBankMemory::s_FsbFiles) {
			if (fsb_file.Offset == exinfo->fileoffset) {
				const auto sample_index = exinfo->inclusionlist >> 1;

				// BOOKMARK-POSSIBLE-OPT
				//*sound = fsb_file.Samples[sample_index].Sound;
				//if (*sound != nullptr) {
				//	return FMOD::OK;
				//}

				const auto& sample = fsb_file.Samples[sample_index];
				if (sample.Buffer.DataSize > 0) {
					static char empty_wav[]{
						"\x52\x49\x46\x46\x25\x00\x00\x00\x57\x41\x56\x45\x66\x6D\x74\x20"
						"\x10\x00\x00\x00\x01\x00\x01\x00\x44\xAC\x00\x00\x88\x58\x01\x00"
						"\x02\x00\x10\x00\x64\x61\x74\x61\x74\x00\x00\x00\x00"
					};

					// Need to specify OPENMEMORY_POINT in case the .bank file is loose
					const FMOD::FMOD_MODE loose_mode = (FMOD::FMOD_MODE)(mode | FMOD::MODE_OPENMEMORY_POINT);

					FMOD::CREATESOUNDEXINFO loose_exinfo{
						.cbsize = sizeof(loose_exinfo),
						.length = sizeof(empty_wav),
						.fileoffset = 0,
						.numsubsounds = 1,
						.inclusionlist = 1
					};
					const auto create_empty_sound_res = Trampoline(fmod_system, empty_wav, loose_mode, &loose_exinfo, sound);

					if (create_empty_sound_res == FMOD::OK) {
						auto get_sub_sound = [](FMOD::Sound* sound, int sub_sound_index) ->FMOD::Sound** {
							const auto* num_sub_sounds = reinterpret_cast<int*>(sound) + 0x2c;
							if (*num_sub_sounds <= sub_sound_index) {
								return nullptr;
							}
							const auto* sub_sounds_array = reinterpret_cast<FMOD::Sound***>(sound) + 0x14;
							return (*sub_sounds_array) + sub_sound_index;
						};

						if (FMOD::Sound** sub_sound = get_sub_sound(*sound, 0))
						{
							if (*sub_sound != nullptr) {
								ReleaseSound(*sub_sound);
							}

							const FMOD::FMOD_MODE loose_sub_sound_mode = (FMOD::FMOD_MODE)(mode | FMOD::MODE_OPENMEMORY_POINT | FMOD::MODE_OPENRAW);

							FMOD::CREATESOUNDEXINFO loose_subsound_exinfo{
								.cbsize = sizeof(loose_subsound_exinfo),
								.length = (std::uint32_t)sample.Buffer.DataSize,
								.numchannels = sample.Buffer.NumChannels,
								.defaultfrequency = sample.Buffer.Frequency,
								.format = [&sample](SoundFormat bits_per_sample) {
									switch (bits_per_sample) {
									default:
										LogInfo("Sound format is not supported for file {}, falling back to original game audio...", sample.Name);
										return FMOD::SOUND_FORMAT::NONE;
									case SoundFormat::PCM_8:
										return FMOD::SOUND_FORMAT::PCM8;
									case SoundFormat::PCM_16:
										return FMOD::SOUND_FORMAT::PCM16;
									case SoundFormat::PCM_24:
										return FMOD::SOUND_FORMAT::PCM24;
									case SoundFormat::PCM_32:
										return FMOD::SOUND_FORMAT::PCM32;
									case SoundFormat::PCM_FLOAT:
										return FMOD::SOUND_FORMAT::PCMFLOAT;
									}
								}(sample.Buffer.Format),
								.numsubsounds = 0
							};

							const auto create_sub_sound_res = Trampoline(fmod_system, (const char*)sample.Buffer.Data.get(), loose_sub_sound_mode, &loose_subsound_exinfo, sub_sound);
							if (create_sub_sound_res == FMOD::OK) {
								return FMOD::OK;
							}

							LogInfo("Failed loading loose audio file {}, falling back to loading from soundbank...", sample.Name);
							[[maybe_unused]] const auto destroy_leaking_sound_res = ReleaseSound(*sound);
							assert(destroy_leaking_sound_res == FMOD::OK);
						}
					}
				}
			}
		}

		return Trampoline(fmod_system, file_name_or_data, mode, exinfo, sound);
	}
};

struct DetourFmodSystemCreateStream {
	inline static SigScan::Function<FMOD::FMOD_RESULT(__stdcall*)(FMOD::System*, const char*, FMOD::FMOD_MODE, FMOD::CREATESOUNDEXINFO*, FMOD::Sound**)> Trampoline{
		.ProcName = "?createStream@System@FMOD@@QEAA?AW4FMOD_RESULT@@PEBDIPEAUFMOD_CREATESOUNDEXINFO@@PEAPEAVSound@2@@Z",
		.Module = "fmod.dll"
	};
	static FMOD::FMOD_RESULT Detour(FMOD::System* fmod_system, const char* file_name_or_data, FMOD::FMOD_MODE mode, FMOD::CREATESOUNDEXINFO* exinfo, FMOD::Sound** sound) {
		LogInfo("Loading an audio stream which has not been patched, falling back to loading original stream...");
		return Trampoline(fmod_system, file_name_or_data, mode, exinfo, sound);
	}
};

struct DetourFmodSystemReleaseSound {
	inline static SigScan::Function<FMOD::FMOD_RESULT(__stdcall*)(FMOD::Sound*)> Trampoline{
		.ProcName = "?release@Sound@FMOD@@QEAA?AW4FMOD_RESULT@@XZ",
		.Module = "fmod.dll"
	};
	static FMOD::FMOD_RESULT Detour(FMOD::Sound* sound) {
		// BOOKMARK-POSSIBLE-OPT
		//static auto get_user_data = [](FMOD::Sound* sound) {
		//	return *(reinterpret_cast<void**>(sound) + 0x19);
		//};
		//const auto user_data = get_user_data(sound);
		//if (user_data == &DetourFmodSystemLoadBankMemory::s_FsbFiles) {
		//	return FMOD::OK;
		//}

		return Trampoline(sound);
	}
};

inline FMOD::FMOD_RESULT CreateSound(FMOD::System* fmod_system, const char* filename_or_data, FMOD::FMOD_MODE mode, FMOD::CREATESOUNDEXINFO* exinfo, FMOD::Sound** sound) {
	return DetourFmodSystemCreateSound::Trampoline(fmod_system, filename_or_data, mode, exinfo, sound);
}

inline FMOD::FMOD_RESULT ReleaseSound(FMOD::Sound* sound) {
	return DetourFmodSystemReleaseSound::Trampoline(sound);
}

std::vector<DetourEntry> GetFmodDetours() {
	const bool enable_loose_audio_files = []() {
		return INIReader("playlunky.ini").GetBoolean("settings", "enable_loose_audio_files", true);
	}();
	if (enable_loose_audio_files) {
		return {
			DetourHelper<DetourFmodSystemLoadBankMemory>::GetDetourEntry("FMOD::System::loadBankMemory"),
			DetourHelper<DetourFmodSystemLoadBankFile>::GetDetourEntry("FMOD::System::loadBankFile"),
			DetourHelper<DetourFmodSystemUnloadBank>::GetDetourEntry("FMOD::Bank::unload"),
			DetourHelper<DetourFmodSystemCreateSound>::GetDetourEntry("FMOD::System::createSound"),
			DetourHelper<DetourFmodSystemCreateStream>::GetDetourEntry("FMOD::System::createStream"),
			DetourHelper<DetourFmodSystemReleaseSound>::GetDetourEntry("FMOD::Sound::release")
		};
	}
	else {
		DetourFmodSystemLoadBankMemory::s_EnableLooseFiles = false;
		return {
			DetourHelper<DetourFmodSystemLoadBankMemory>::GetDetourEntry("FMOD::System::loadBankMemory"),
			DetourHelper<DetourFmodSystemLoadBankFile>::GetDetourEntry("FMOD::System::loadBankFile")
		};
	}
}

void SetFmodVfs(VirtualFilesystem* vfs) {
	s_FmodVfs = vfs;
}
