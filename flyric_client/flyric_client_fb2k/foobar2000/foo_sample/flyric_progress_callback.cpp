#include "stdafx.h"

#define ERROR_PREFIX "[error]flyric_progress_callback:"

#define UDP_DATA_TYPE_LOADLYRIC     1
#define UDP_DATA_TYPE_PLAY_TIME     2
#define UDP_DATA_TYPE_PAUSE_TIME    3
#define UDP_DATA_TYPE_PLAY_NOW      4
#define UDP_DATA_TYPE_PAUSE_NOW     5

#include <atltime.h>
#include <atlsocket.h>


struct PackageData {
	char* data;
	int len;
};

class flyric_progress_callback : public play_callback_static {
private:

	static HANDLE mutex;
	static SOCKET send_socket;
	static DWORD WINAPI SendPackageThreadFunc(LPVOID data);

	static void setInt(char* data, int32_t i);
	static void setLong(char* data, int64_t i);

	static uint64_t getTime();

	static int send_id;

	static PackageData* makePackage(int id, int len);


	void sendPackage(PackageData* pdata) {
		//fill id here
		if (mutex == NULL) {
			mutex = CreateMutex(NULL, FALSE, NULL);
			if (mutex == NULL) {
				console::formatter() << ERROR_PREFIX"CreateMutex error:" << GetLastError();
				return;
			}
		}
		if (send_socket == INVALID_SOCKET) {
			send_socket = socket(AF_INET, SOCK_DGRAM, 0);
			if (send_socket == INVALID_SOCKET) {
				console::formatter() << ERROR_PREFIX"socket create error:" << WSAGetLastError();
			}
		}
		CreateThread(NULL, 0, &SendPackageThreadFunc, pdata, 0, NULL);
	}

	
public:

	void on_playback_starting(play_control::t_track_command p_command, bool p_paused) {}
	void on_playback_new_track(metadb_handle_ptr p_track) {
		report_load(get_title());
		report_play(0);
	}
	void on_playback_stop(play_control::t_stop_reason p_reason) {
		report_pause(0);
	}
	void on_playback_seek(double p_time) {
		if (playback_control::get()->is_paused())
			report_pause(int64_t(p_time * 1000));
		else
			report_play(int64_t(p_time * 1000));
	}
	void on_playback_pause(bool p_state) {
		if (p_state) {
			report_pause(int64_t(playback_control::get()->playback_get_position() * 1000));
		}
		else {
			report_play(int64_t(playback_control::get()->playback_get_position() * 1000));
		}
	}
	void on_playback_edited(metadb_handle_ptr p_track) {}
	void on_playback_dynamic_info(const file_info& p_info) {}
	void on_playback_dynamic_info_track(const file_info& p_info) {}
	void on_playback_time(double p_time) {
		if (playback_control::get()->is_paused())
			report_pause(int64_t(p_time * 1000));
		else
			report_play(int64_t(p_time * 1000));
	}
	void on_volume_change(float p_new_val) {}

	unsigned get_flags() {
		return flag_on_playback_all;
	}

private:
	bool isLogToConsole() {
		return true;
	}

	pfc::string get_title() {
		if (m_titleformat_obj.is_empty()) {
			static_api_ptr_t<titleformat_compiler>()->compile_safe_ex(m_titleformat_obj, "%title%");
		}
		pfc::string_formatter output;
		playback_control::get()->playback_format_title(NULL, output, m_titleformat_obj, NULL, playback_control::display_level_all);
		return output;
	}

	void report_load(pfc::string str) {
		//i don't know the encoding of str,maybe utf8
		int id = send_id = 0;
		PackageData* data = makePackage(id, 4 + 4 + str.length() + 1);
		setInt(data->data + 4, UDP_DATA_TYPE_LOADLYRIC);
		const char* dt = str.c_str();
		for (int i = 0, e = str.length(); i < e; i++) {
			data->data[4 + 4 + i] = dt[i];
		}
		data->data[4 + 4 + str.length()] = '\0';

		if (isLogToConsole())
			console::formatter() << "report load music:" << (data->data + 4);
		sendPackage(data);
	}

	void report_play(int64_t music_progress) {
		int64_t pbeg = (int64_t)(getTime() - music_progress);
		PackageData* pkg = makePackage(++send_id, 4 + 4 + 8);
		setInt(pkg->data + 4, UDP_DATA_TYPE_PLAY_TIME);
		setLong(pkg->data + 8, pbeg);

		if (isLogToConsole()) 
			console::formatter() << "Seek as play,begin time:" << pbeg;
		

		sendPackage(pkg);
	}

	void report_pause(int64_t music_progress) {
		int64_t pbeg = music_progress;
		PackageData* pkg = makePackage(++send_id, 4 + 4 + 8);
		setInt(pkg->data + 4, UDP_DATA_TYPE_PAUSE_TIME);
		setLong(pkg->data + 4 + 4, pbeg);

		if (isLogToConsole()) {
			console::formatter() << "Seek as paused,begin time:" << pbeg;
		}

		sendPackage(pkg);
	}

	titleformat_object::ptr m_titleformat_obj;
};

HANDLE flyric_progress_callback::mutex = NULL;
SOCKET flyric_progress_callback::send_socket = INVALID_SOCKET;
int flyric_progress_callback::send_id = 0;

DWORD WINAPI flyric_progress_callback::SendPackageThreadFunc(LPVOID data) {

	PackageData* pdata = (PackageData*)data;
	WaitForSingleObject(mutex, INFINITE);
	//safe zone

	
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	//a warring C4996.I don't care.
	addr.sin_addr.S_un.S_addr = inet_addr("127.0.0.1");
	addr.sin_port = htons(9588);
	if (sendto(send_socket, pdata->data, pdata->len, 0, (sockaddr*)& addr, sizeof(addr)) < 0) {
		console::formatter() << ERROR_PREFIX"Socket send error:" << errno;
	}
#if 0
	{
		pfc::string8 f;
		f << "Send data len="<<(pdata->len)<<":";

		for (int i = 0; i < pdata->len; i++) {
			f << (int)pdata->data[i];
			f << " ";
		}
		console::print(f);
	}
#endif

	if (!ReleaseMutex(mutex)) {
		console::formatter() << ERROR_PREFIX"ReleaseMutex error:" << GetLastError();
	}
	delete[]pdata->data;
	delete pdata;
	return 0;
}

void flyric_progress_callback::setInt(char* data, int32_t i) {
	data[3] = char(i);
	data[2] = char(i >> 8);
	data[1] = char(i >> 16);
	data[0] = char(i >> 24);
}
void flyric_progress_callback::setLong(char* data, int64_t i) {
	data[7] = char(i);
	data[6] = char(i >> 8);
	data[5] = char(i >> 16);
	data[4] = char(i >> 24);
	data[3] = char(i >> 32);
	data[2] = char(i >> 40);
	data[1] = char(i >> 48);
	data[0] = char(i >> 56);
}

uint64_t flyric_progress_callback::getTime() {
	static bool init = false;
	static CFileTime init_time;
	if (!init) {
		init = true;
		SYSTEMTIME dtime = { 1970,1,4,1,0,0,0,0 };
		FILETIME ftime;
		SystemTimeToFileTime(&dtime, &ftime);
		init_time = ftime;
	}
	auto r = (CFileTime::GetCurrentTime() - init_time).GetTimeSpan();
	r /= 10000;// miliseconed??? Did I read wrong document? https://docs.microsoft.com/en-us/cpp/atl-mfc-shared/reference/cfiletimespan-class?view=vs-2019#gettimespan
	return r;
}
PackageData* flyric_progress_callback::makePackage(int id, int len) {
	PackageData* ret = new PackageData;
	ret->data = new char[len];
	ret->len = len;
	setInt(ret->data, id);
	return ret;
}
static play_callback_static_factory_t<flyric_progress_callback> g_flyric_progress_callback;