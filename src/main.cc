//should check getlasterror after syscalls
//testing stage where I use the program for a week

//SETTINGS:
//add ability to show/hide currently focused window.

//Extra fun stuff
//Should be able to scale dpi on the fly with multiple windows.
//Be able to focus on multiple different windows

#define GB_IMPLEMENTATION
#include "pch.h"
#undef GB_IMPLEMENTATION

#define CUTE_PATH_IMPLEMENTATION
#include "cute/cute_path.h"
#undef CUTE_PATH_IMPLEMENTATION

#include "io.h"
#include "icon.h"

using namespace sf;
using namespace glm;

#define data_dir "data/"
#define file_name "sessiondata.csv"

int win_height = VideoMode::getDesktopMode().height/2;
int win_width  = win_height;

RenderWindow win;

gbAllocator a = gb_heap_allocator();

gbArray(gbString) window_names;

gbArray(HWND) window_hwnds;

//Name of the exe we want to focus on
char focus_exe_string[FILENAME_MAX];

const int min_session_write_time_s = 60;

time_t session_start_timestamp = 0;
time_t session_end_timestamp = 0;
float focus_time_s = 0.0f;
float slack_time_s = 0.0f;
float afk_time_s = 0.0f;

bool in_session = false;
bool in_break = false;

//Time since you last made an input
float time_since_last_input_s = 0.0f;

//How long it takes from no input to be considerd afk
float time_from_no_input_to_afk_s = 5*60;

bool check_all_keys_pressed() {
	for(int i = Keyboard::A; i != Keyboard::KeyCount; i++) {
		if(Keyboard::isKeyPressed((Keyboard::Key)i)) {
			return true;
		}
	}
	return false;
}

void convert_seconds_to_hms_int(float seconds, int* out_hours, int* out_minutes, int* out_seconds) {
	*out_hours = int(seconds/3600);
	*out_minutes = int((seconds - 3600*(*out_hours))/60);
	*out_seconds = int(seconds - 3600*(*out_hours) - 60*(*out_minutes));
}

void remove_trailing_newline(char *str) {
	if(str == NULL)
		return;
	int length = strlen(str);
	if(str[length-1] == '\n')
		str[length-1] = '\0';
}

bool get_exe_name_from_HWND(HWND window, char* exe_name) {
	DWORD value = MAX_PATH;
	static char full_path[MAX_PATH];

	DWORD process_id = null;
	GetWindowThreadProcessId(window, &process_id);
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, false, process_id);
	QueryFullProcessImageName(hProcess, 0, full_path, &value);

	//I should be doing getlasterror here
	if(process_id == null || hProcess == null || full_path[0] == null)
	{
		return false;
	}

	path_pop(full_path, null, exe_name);
	return true;
}

bool out_session(FILE* fp) {
	struct tm* buf;
	char str1[26];
	char str2[26];

	buf = localtime(&session_start_timestamp);
	snprintf(str1, sizeof(str1), "%s", asctime(buf));

	buf = localtime(&session_end_timestamp);
	snprintf(str2, sizeof(str2), "%s", asctime(buf));
	remove_trailing_newline(str1);
	remove_trailing_newline(str2);

	if(fprintf(fp, "%s,%s,%i,%i,%i\n", str1, str2, (int)focus_time_s, (int)slack_time_s, (int)afk_time_s)) {
		return true;
	}

	return false;
}

bool write_session(const char* path) {
	//Check if file exists
	FILE* fp = fopen(path, "r");
	if(fp == NULL) {
		fp = fopen(path, "w+");
		fprintf(fp, "startTime,endTime,focusTimeSec,slackTimeSec,AFKTimeSec\n");
	}
	else {
		fclose(fp);
		fp = fopen(path, "a+");
	}

	asshurt_f(fp != NULL, "failed to open writeable file '%s'.", path) { return false; }

	const bool io_success =
		out_session(fp) &&
		true;
	fclose(fp);

	asshurt_f(io_success, "failed to save %s: %s", path) { return false; }

	return io_success;
}

bool end_session() {
	//Don't save sessions less than 60s long
	if(in_session && (time(NULL) - session_start_timestamp > min_session_write_time_s)) {
		session_end_timestamp = time(NULL);

		//write all 3 time floats as well as timestamp for starting sesh and ending
		write_session(data_dir file_name);
		focus_time_s = slack_time_s = afk_time_s = 0.0f;
		return true;
	}
	return false;
}

static BOOL CALLBACK enum_window_callback_copy_hwnd(HWND hWnd, LPARAM lparam) {

	int length = GetWindowTextLength(hWnd);
    // List visible windows with a non-empty title
    if (IsWindowVisible(hWnd) && length != 0) {
		gb_array_append(window_hwnds, hWnd);
    }
    return TRUE;
}

void get_window_strings_and_hwnd() {
	gb_array_clear(window_hwnds);

	for(int i = 0; i < gb_array_count(window_names); ++i) {
		gb_string_free(window_names[i]);
	}
	gb_array_clear(window_names);

	EnumWindows(enum_window_callback_copy_hwnd, null);
	
	for(int i = 0; i < gb_array_count(window_hwnds); ++i) {
		int win_name_length = GetWindowTextLength(window_hwnds[i]);
		static char temp[64];
		GetWindowText(window_hwnds[i], temp, asize(temp));
		gbString name = gb_string_make(a, "");
		name = gb_string_set(name, temp);
		gb_array_append(window_names, name);
	}
}

int main() {
	defer(end_session(););
	defer(ImGui::SFML::Shutdown(););

	gb_array_init(window_hwnds, a);
	gb_array_init(window_names, a);
	defer(gb_array_free(window_hwnds));
	defer(
		for(int i = 0; i < gb_array_count(window_names); ++i) {
			gb_string_free(window_names[i]);
		}
	    gb_array_free(window_names);
	);

	get_window_strings_and_hwnd();

	int win_style = Style::Close | Style::Titlebar;

	win.create(VideoMode(win_width, win_height), "TrackFocus", win_style);
	win.setIcon(32, 32, sfml_icon.pixel_data);

	ImGui::SFML::Init(win);

	//Small text should be 5% of the windows height
	const float small_text_height_px = win_height*0.05f;
	const float big_text_height_px = win_height*0.08f;
	const float imgui_size_scale = (float)win_height*0.0025f;

    ImGuiIO& io = ImGui::GetIO();
	ImFont* small_arial_font = io.Fonts->AddFontFromFileTTF(data_dir "Arial.ttf", small_text_height_px);
	ImFont* large_arial_font = io.Fonts->AddFontFromFileTTF(data_dir "Arial.ttf", big_text_height_px);
	ImGui::SFML::UpdateFontTexture();

	ImGuiStyle& style = ImGui::GetStyle();
	style.ScaleAllSizes(imgui_size_scale);
	style.WindowRounding = 0.0f;

	Clock dtc;
	while(win.isOpen()) {
		defer(win.display(););
		defer(ImGui::SFML::Render(win););

		Event e;
		while(win.pollEvent(e)) {
			switch(e.type) {
			case Event::EventType::Closed: { win.close(); } break;
			}
			ImGui::SFML::ProcessEvent(e);
		}

		{
			static Vector2i mouse_prev;
			Vector2i mouse_delta = Mouse::getPosition() - mouse_prev;
			if(mouse_delta.x || mouse_delta.y) { time_since_last_input_s = 0.0f; }
			mouse_prev = Mouse::getPosition();
		}

		const Time dt = dtc.restart();
		static float accumulated_time_s = 0.0f;
		accumulated_time_s += dt.asSeconds();

		float query_period_s = 1.0f;

		if(accumulated_time_s > query_period_s) {

			//If our focus string is not empty
			if(focus_exe_string[0] != null && in_session && !in_break) {

				char foreground_window_exe_string[FILENAME_MAX] = { 0 };

				HWND foreground_window = GetForegroundWindow();

				get_exe_name_from_HWND(foreground_window, foreground_window_exe_string);

				//if we aren't afk	
				if(time_since_last_input_s < time_from_no_input_to_afk_s) {

					//if our focus exe is in the foreground
					if(strncmp(foreground_window_exe_string, focus_exe_string, FILENAME_MAX) == 0) {
						focus_time_s += accumulated_time_s;
					} else {
						slack_time_s += accumulated_time_s;
					}
					static char message_text[64] = { 0 };

					if(win.getSystemHandle() == foreground_window) {
						win.setTitle("TrackFocus");
					} else {
						static char name_string[100];
						GetWindowText(foreground_window, name_string, asize(name_string));
						snprintf(message_text, asize(message_text), "TrackFocus - %s", name_string);
						win.setTitle(message_text);
					}

				} else {
					afk_time_s += accumulated_time_s;
					win.setTitle("TrackFocus - AFK");
				}

				time_since_last_input_s += accumulated_time_s;
			}

			if(check_all_keys_pressed()) {
				time_since_last_input_s = 0.0f;
			}
			accumulated_time_s = 0.0f;
		}

		//Imgui
		win.clear();

		ImGui::SFML::Update(win, dt);

		ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
		ImGui::SetNextWindowSize(ImVec2((float)win_width, (float)win_height), ImGuiCond_Always);

		ImGuiWindowFlags window_flags = 0;
		window_flags |= ImGuiWindowFlags_NoTitleBar;
		window_flags |= ImGuiWindowFlags_NoScrollbar;
		window_flags |= ImGuiWindowFlags_NoMove;
		window_flags |= ImGuiWindowFlags_NoResize;
		window_flags |= ImGuiWindowFlags_NoCollapse;
		window_flags |= ImGuiWindowFlags_NoNav;

		if(ImGui::Begin("main", 0, window_flags)) {
			defer(ImGui::End(););
			ImGui::PushFont(small_arial_font);

			static bool show_afk_timer = true;
			static bool show_slack_timer = true;
			static bool show_seconds_in_timer = true;

			if(ImGui::BeginTabBar("TabBar", ImGuiTabBarFlags_None)) {
				defer(ImGui::PopFont(););
				defer(ImGui::EndTabBar(););

				if(ImGui::BeginTabItem("Focus Window")) {
					defer(ImGui::EndTabItem(););

					static int item_current_i = -1;
					static int saved_i = -2;

					ImGui::PushItemWidth(-1);
					ImGui::PushFont(large_arial_font);

					ImGui::Text("Select window of focus:");
					ImGui::PopFont();
					ImGui::ListBox(" ", &item_current_i, window_names, gb_array_count(window_hwnds), 8);

					//Grey out save button
					const bool grey_out = (item_current_i == -1 || saved_i == item_current_i);
					{
						if(grey_out) {
							ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
							ImGui::PushStyleVar(ImGuiStyleVar_Alpha, ImGui::GetStyle().Alpha * 0.5f);
						}
						defer(if(grey_out) {
							ImGui::PopItemFlag();
							ImGui::PopStyleVar();
						});
						if(ImGui::Button("Save")) {
							if(get_exe_name_from_HWND(window_hwnds[item_current_i], focus_exe_string))
							{
								saved_i = item_current_i;
							}
							else {
								ImGui::OpenPopup("Window no longer exists!");
							}
						}
					}
					// Always center this window when appearing
					ImVec2 center(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f);
					ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
					static float popup_width = ImGui::GetIO().DisplaySize.x * 0.6f;
						ImGui::SetNextWindowSize(ImVec2(popup_width, ImGui::GetIO().DisplaySize.y * 0.25f));

					if(ImGui::BeginPopupModal("Window no longer exists!", NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
						defer(ImGui::EndPopup(););
						const float button_width = popup_width*0.4f;
						ImGui::SameLine(popup_width*0.5f - button_width*0.5f);

						if(ImGui::Button("OK", ImVec2(button_width, 0))) { ImGui::CloseCurrentPopup(); }
					}

					ImGui::SameLine();
					if(ImGui::Button("Refresh window list")) {
						get_window_strings_and_hwnd();
					}

					if(focus_exe_string[0] != null) {
						ImGui::Text("Focus window:\n%s", focus_exe_string);
						if(ImGui::Button(in_session ? "End Session" : "Begin Session")) {
							if(!in_session) {
								//We are starting a session
								session_start_timestamp = time(NULL);
							}
							else {
								//We are ending a session
								end_session();
							}
							in_session = !in_session;
						}
					}
				}

				if(ImGui::BeginTabItem("Timers")) {
					defer(ImGui::EndTabItem(););
					
					const char* timer_format = show_seconds_in_timer ? "%i:%i:%i\n\n" : "%i:%i\n\n";

					int hours = 0, minutes = 0, seconds = 0;
					{
						ImGui::PushFont(large_arial_font);
						defer(ImGui::PopFont(););

						convert_seconds_to_hms_int(focus_time_s, &hours, &minutes, &seconds);
						ImGui::Text("Focus Time:\n");
						ImGui::Text(timer_format, hours, minutes, seconds);

						if(show_slack_timer) {

							convert_seconds_to_hms_int(slack_time_s, &hours, &minutes, &seconds);
							ImGui::Text("Slack Time:");
							ImGui::Text(timer_format, hours, minutes, seconds);
						}
						if(show_afk_timer) {

							convert_seconds_to_hms_int(afk_time_s, &hours, &minutes, &seconds);
							ImGui::Text("AFK Time:");
							ImGui::Text(timer_format, hours, minutes, seconds);
						}
					}

					if(in_session) {
						if(ImGui::Button(in_break ? "End break" : "Start break")) {
							in_break = !in_break;
						}
					}
				}

				if(ImGui::BeginTabItem("Settings")) {
					defer(ImGui::EndTabItem(););

					static int query_period_int_s = 1;
					ImGui::Text("Query period(seconds):");
					bool edited = false;
					edited |= ImGui::RadioButton("1", &query_period_int_s, 1); ImGui::SameLine();
					edited |= ImGui::RadioButton("2", &query_period_int_s, 2); ImGui::SameLine();
					edited |= ImGui::RadioButton("5", &query_period_int_s, 5);
					if(edited) { query_period_s = (float)query_period_int_s; }

					static int AFK_time_int_s = 60*5;
					ImGui::Text("Time to AFK(minutes):");
					edited = false;
					edited |= ImGui::RadioButton("3", &AFK_time_int_s, 60*2); ImGui::SameLine();
					edited |= ImGui::RadioButton("5\r", &AFK_time_int_s, 60*5); ImGui::SameLine();
					edited |= ImGui::RadioButton("10", &AFK_time_int_s, 60*10);
					if(edited) { time_from_no_input_to_afk_s = (float)AFK_time_int_s; }

					ImGui::Checkbox("Show slack timer", &show_slack_timer);
					ImGui::Checkbox("Show AFK timer", &show_afk_timer);
					ImGui::Checkbox("Show seconds in timer", &show_seconds_in_timer);

					if(ImGui::Button("Go to my websight")) {
						ShellExecute(NULL, "open", "https://www.behance.net/ethantoly", "", ".", SW_SHOWDEFAULT);
					}
				}
			}
		}
	}
	return 0;
}