// The UI was 99% AI generated.
#include <wx/wx.h>
#include <wx/listbox.h>
#include <wx/scrolbar.h>
#include <wx/graphics.h>
#include <wx/dcbuffer.h>
#include <filesystem>
#include <vector>
#include <string>
#include <thread>
#include "PlayerFunctionality.h"
#include "resource.h"
#include "VersionNum.h"
#include "globals.h"

#ifdef __WXMSW__
#include <wx/msw/private.h>
#include <windows.h>
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#endif

bool obstruction = true;

class DarkPanel : public wxPanel {
public:
    DarkPanel(wxWindow* parent, wxWindowID id = wxID_ANY)
        : wxPanel(parent, id)
    {
        SetBackgroundColour(wxColour(35, 35, 35));  // Slightly lighter than main background

        Bind(wxEVT_PAINT, &DarkPanel::OnPaint, this);
    }

private:
    void OnPaint(wxPaintEvent& evt) {
        wxPaintDC dc(this);
        wxSize size = GetSize();

        // Draw the border
        dc.SetPen(wxPen(wxColour(60, 60, 60), 1));  // Subtle border color
        dc.SetBrush(*wxTRANSPARENT_BRUSH);
        dc.DrawRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), 4);
    }
};

class DarkListBox : public wxListBox {
public:
    DarkListBox(wxWindow* parent, wxWindowID id)
        : wxListBox(parent, id, wxDefaultPosition, wxDefaultSize,
            0, NULL, wxLB_SINGLE | wxNO_BORDER | wxLB_NO_SB)
    {
        SetBackgroundColour(wxColour(40, 40, 40));
        SetForegroundColour(wxColour(200, 200, 200));



        Bind(wxEVT_MOUSEWHEEL, &DarkListBox::OnMouseWheel, this);
    }

protected:
    void OnMouseWheel(wxMouseEvent& event) {
        int nScrollLines = 3;
        if (event.GetWheelRotation() > 0) {
            for (int i = 0; i < nScrollLines; i++) {
                ScrollLines(-1);
            }
        }
        else {
            for (int i = 0; i < nScrollLines; i++) {
                ScrollLines(1);
            }
        }
    }
};
class MainFrame;

// Custom Button with dark theme
class DarkButton : public wxButton {
public:
    DarkButton(wxWindow* parent, wxWindowID id, const wxString& label)
        : wxButton(parent, id, label, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE | wxNO_BORDER)
        , m_isHovered(false)
        , m_isPressed(false)
        , m_cornerRadius(6)
    {
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetBackgroundColour(wxColour(60, 60, 60));
        SetForegroundColour(wxColour(200, 200, 200));

        // Bind all relevant events
        Bind(wxEVT_PAINT, &DarkButton::OnPaint, this);
        Bind(wxEVT_ENTER_WINDOW, &DarkButton::OnEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &DarkButton::OnLeave, this);
        Bind(wxEVT_LEFT_DOWN, &DarkButton::OnMouseDown, this);
        Bind(wxEVT_LEFT_UP, &DarkButton::OnMouseUp, this);
        Bind(wxEVT_ERASE_BACKGROUND, &DarkButton::OnEraseBackground, this);
        Bind(wxEVT_BUTTON, &DarkButton::OnButtonClick, this);

        SetMinSize(wxSize(60, 30));
    }

    void ForceRefresh() {
        m_isPressed = false;
        m_isHovered = false;
        Refresh(false); 
        Update();
    }

protected:
    void OnEraseBackground(wxEraseEvent& event) {
        // Do nothing to prevent flickering
    }

    void OnPaint(wxPaintEvent& event) {
        wxBufferedPaintDC dc(this);
        wxSize size = GetSize();

        wxGraphicsContext* gc = wxGraphicsContext::Create(dc);
        if (gc) {
            // Clear the background
            gc->SetBrush(GetParent()->GetBackgroundColour());
            gc->SetPen(*wxTRANSPARENT_PEN);
            gc->DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

            // Determine button color based on all states
            wxColour bgColor;
            if (GetBackgroundColour() == wxColour(147, 112, 219)) {
                // Special color for selected speed buttons
                if (m_isPressed) {
                    bgColor = wxColour(137, 102, 209); // Darker when pressed
                }
                else if (m_isHovered) {
                    bgColor = wxColour(157, 122, 229); // Lighter when hovered
                }
                else {
                    bgColor = wxColour(147, 112, 219); // Normal
                }
            }
            else {
                // Regular button colors
                if (m_isPressed) {
                    bgColor = wxColour(50, 50, 50); // Darker when pressed
                }
                else if (m_isHovered) {
                    bgColor = wxColour(80, 80, 80); // Lighter when hovered
                }
                else {
                    bgColor = wxColour(60, 60, 60); // Normal
                }
            }

            // Draw the rounded button
            gc->SetBrush(wxBrush(bgColor));
            gc->SetPen(*wxTRANSPARENT_PEN);
            wxGraphicsPath path = gc->CreatePath();
            path.AddRoundedRectangle(0, 0, size.GetWidth(), size.GetHeight(), m_cornerRadius);
            gc->FillPath(path);

            // Draw text
            gc->SetFont(GetFont(), GetForegroundColour());
            wxString label = GetLabel();
            double textWidth, textHeight;
            gc->GetTextExtent(label, &textWidth, &textHeight);

            double x = (size.GetWidth() - textWidth) / 2;
            double y = (size.GetHeight() - textHeight) / 2;
            // Slight offset when pressed for a "pressed" effect
            if (m_isPressed) {
                x += 1;
                y += 1;
            }
            gc->DrawText(label, x, y);

            delete gc;
        }
    }

    void OnMouseDown(wxMouseEvent& event) {
        m_isPressed = true;
        Refresh();
        event.Skip();
    }

    void OnMouseUp(wxMouseEvent& event) {
        m_isPressed = false;
        Refresh();
        event.Skip();
    }

    void OnButtonClick(wxCommandEvent& event) {
        RefreshAllButtonsInWindow(GetParent());
        event.Skip();
    }

    void OnEnter(wxMouseEvent& event) {
        m_isHovered = true;
        Refresh();
        event.Skip();
    }

    void OnLeave(wxMouseEvent& event) {
        m_isHovered = false;
        m_isPressed = false;  // Reset pressed state when mouse leaves
        Refresh();
        event.Skip();
    }

private:
    void RefreshAllButtonsInWindow(wxWindow* window) {
        // Refresh buttons in the current window
        wxWindowList& children = window->GetChildren();
        for (wxWindow* child : children) {
            DarkButton* button = dynamic_cast<DarkButton*>(child);
            if (button) {
                button->ForceRefresh();
            }
            // Recursively check children windows
            RefreshAllButtonsInWindow(child);
        }
    }

    bool m_isHovered;
    bool m_isPressed;
    int m_cornerRadius;
};

class CustomProgressBar;
wxDEFINE_EVENT(wxEVT_PROGRESS_SEEK, wxCommandEvent);



// Custom progress bar class
class CustomProgressBar : public wxControl {
public:
    CustomProgressBar(wxWindow* parent, wxWindowID id, int range = 100)
        : wxControl(parent, id, wxDefaultPosition, wxDefaultSize, wxBORDER_NONE)
        , m_range(range)
        , m_value(0)
        , m_isHovering(false)
    {
        SetBackgroundColour(wxColour(40, 40, 40));
        SetMinSize(wxSize(-1, 12));

        Bind(wxEVT_PAINT, &CustomProgressBar::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &CustomProgressBar::OnMouseClick, this);
        Bind(wxEVT_MOTION, &CustomProgressBar::OnMouseMove, this);
        Bind(wxEVT_ENTER_WINDOW, &CustomProgressBar::OnMouseEnter, this);
        Bind(wxEVT_LEAVE_WINDOW, &CustomProgressBar::OnMouseLeave, this);

        // Create cursor
        SetCursor(wxCursor(wxCURSOR_HAND));
    }

    void SetValue(int value) {
        m_value = value;
        Refresh();
    }

    void SetRange(int range) {
        m_range = range;
        Refresh();
    }

    wxDECLARE_EVENT_TABLE();
    wxDECLARE_DYNAMIC_CLASS(CustomProgressBar);

private:
    void OnPaint(wxPaintEvent& evt) {
        wxPaintDC dc(this);
        wxSize size = GetSize();

        // Draw background
        dc.SetBrush(wxBrush(GetBackgroundColour()));
        dc.SetPen(*wxTRANSPARENT_PEN);
        dc.DrawRectangle(0, 0, size.GetWidth(), size.GetHeight());

        if (m_range > 0) {
            // Calculate progress width
            int progressWidth = (size.GetWidth() * m_value) / m_range;

            // Create gradient colors
            wxColour startColor(147, 112, 219);
            wxColour endColor(122, 93, 182);

            if (m_isHovering) {
                // Brighten colors when hovering
                startColor = startColor.ChangeLightness(110);
                endColor = endColor.ChangeLightness(110);
            }

            // Draw progress with gradient
            if (progressWidth > 0) {
                wxRect progressRect(0, 0, progressWidth, size.GetHeight());
                dc.GradientFillLinear(progressRect, startColor, endColor, wxEAST);
            }

            // Draw hover position indicator
            if (m_isHovering) {
                dc.SetPen(wxPen(wxColour(255, 255, 255, 180), 2));
                dc.DrawLine(m_hoverX, 0, m_hoverX, size.GetHeight());
            }
        }
    }

    void OnMouseClick(wxMouseEvent& event) {
        if (m_range > 0) {
            int newValue = (event.GetX() * m_range) / GetSize().GetWidth();
            newValue = wxMin(wxMax(newValue, 0), m_range);

            // Create and send a custom seek event
            wxCommandEvent seekEvent(wxEVT_COMMAND_BUTTON_CLICKED, GetId());
            seekEvent.SetInt(newValue);
            seekEvent.SetEventObject(this);
            ProcessEvent(seekEvent);
        }
    }

    void OnMouseMove(wxMouseEvent& event) {
        if (m_isHovering) {
            m_hoverX = event.GetX();
            Refresh();
        }
    }

    void OnMouseEnter(wxMouseEvent& event) {
        m_isHovering = true;
        m_hoverX = event.GetX();
        Refresh();
    }

    void OnMouseLeave(wxMouseEvent& event) {
        m_isHovering = false;
        Refresh();
    }

    int m_range;
    int m_value;
    bool m_isHovering;
    int m_hoverX;
};

IMPLEMENT_CLASS(CustomProgressBar, wxControl)
wxBEGIN_EVENT_TABLE(CustomProgressBar, wxControl)
EVT_PAINT(CustomProgressBar::OnPaint)
EVT_LEFT_DOWN(CustomProgressBar::OnMouseClick)
EVT_MOTION(CustomProgressBar::OnMouseMove)
EVT_ENTER_WINDOW(CustomProgressBar::OnMouseEnter)
EVT_LEAVE_WINDOW(CustomProgressBar::OnMouseLeave)
wxEND_EVENT_TABLE()

class DarkSearchBox : public wxTextCtrl {
public:
    DarkSearchBox(wxWindow* parent, wxWindowID id)
        : wxTextCtrl(parent, id, wxEmptyString, wxDefaultPosition, wxDefaultSize,
            wxTE_PROCESS_ENTER | wxBORDER_NONE)
    {
        SetBackgroundColour(wxColour(50, 50, 50));
        SetForegroundColour(wxColour(200, 200, 200));

        // Set placeholder text color
        SetHint("Search songs...");

        // Custom font
        wxFont customFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false, "Segoe UI");
        SetFont(customFont);
    }
};


class MainFrame : public wxFrame {
public:
    MainFrame() : wxFrame(nullptr, wxID_ANY, "Guitar Player v" + std::string(VERSIONSTR),
        wxDefaultPosition, wxSize(300, 500),
        wxMINIMIZE_BOX | wxSYSTEM_MENU | wxCAPTION | wxCLOSE_BOX | wxCLIP_CHILDREN),
        isPlayingSong(false), isPaused(false), currentProgress(0)
    {
        // Enable dark title bar for Windows
#ifdef __WXMSW__
        HWND hwnd = (HWND)GetHandle();
        if (hwnd) { // Enable dark mode for title bar
            BOOL value = TRUE;
            DwmSetWindowAttribute(hwnd, DWMWA_USE_IMMERSIVE_DARK_MODE, &value, sizeof(value));
            HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
            if (hIcon)
            {
                // Set the small icon (16x16)
                SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
                // Set the big icon (32x32)
                SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
            }


        }
#endif

        SetBackgroundColour(wxColour(30, 30, 30));

        // Create main sizer
        wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);

        DarkPanel* groupPanelSongs = new DarkPanel(this);
        wxBoxSizer* groupSizerSongs = new wxBoxSizer(wxVERTICAL);
        wxBoxSizer* searchSizer = new wxBoxSizer(wxHORIZONTAL);

        searchBox = new DarkSearchBox(groupPanelSongs, wxID_ANY);
        refreshButton = new DarkButton(groupPanelSongs, wxID_ANY, "Refresh");
        obstructionButton = new DarkButton(groupPanelSongs, wxID_ANY, "Obstruct");

        // Add search box and refresh button to the horizontal sizer
        searchSizer->Add(obstructionButton, 0, wxEXPAND | wxRIGHT, 5);
        searchSizer->Add(searchBox, 1, wxEXPAND | wxRIGHT, 5);
        searchSizer->Add(refreshButton, 0, wxEXPAND | wxRIGHT, 5);
        

        listBox = new DarkListBox(groupPanelSongs, wxID_ANY);

        LoadSongs("songs");

        groupSizerSongs->Add(searchSizer, 0, wxEXPAND | wxALL, 10);
        groupSizerSongs->Add(listBox, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);
        groupPanelSongs->SetSizer(groupSizerSongs);
        mainSizer->Add(groupPanelSongs, 1, wxEXPAND | wxALL, 10);


        

        DarkPanel* playbackPanel = new DarkPanel(this);
        wxBoxSizer* playbackSizer = new wxBoxSizer(wxVERTICAL);


        currentSongLabel = new wxStaticText(playbackPanel, wxID_ANY, "No song selected");
        currentSongLabel->SetForegroundColour(wxColour(200, 200, 200));
        //currentSongLabel->SetWindowStyle(wxALIGN_CENTER_HORIZONTAL);

        progressBar = new CustomProgressBar(playbackPanel, wxID_ANY);
        timeLabel = new wxStaticText(playbackPanel, wxID_ANY, "0:00 / 0:00");
        timeLabel->SetForegroundColour(wxColour(200, 200, 200));



        wxBoxSizer* buttonSizer = new wxBoxSizer(wxHORIZONTAL);
        playButton = new DarkButton(playbackPanel, wxID_ANY, "Play");
        pauseButton = new DarkButton(playbackPanel, wxID_ANY, "Pause");
        stopButton = new DarkButton(playbackPanel, wxID_ANY, "Stop");

        buttonSizer->Add(playButton, 1, wxRIGHT, 5);
        buttonSizer->Add(pauseButton, 1, wxLEFT | wxRIGHT, 5);
        buttonSizer->Add(stopButton, 1, wxLEFT, 5);

        // Create speed controls
        CreateSpeedControls(playbackPanel);  // Modified to take parent parameter


        playbackSizer->Add(currentSongLabel, 0, wxEXPAND | wxALL, 10);
        playbackSizer->Add(progressBar, 0, wxEXPAND | wxLEFT | wxRIGHT, 10);
        playbackSizer->Add(timeLabel, 0, wxALIGN_CENTER | wxALL, 5);
        playbackSizer->Add(buttonSizer, 0, wxEXPAND | wxALL, 10);
        playbackSizer->Add(speedSizer, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

        playbackPanel->SetSizer(playbackSizer);
        mainSizer->Add(playbackPanel, 0, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, 10);

        SetSizer(mainSizer);

        // Bind events
        searchBox->Bind(wxEVT_TEXT, &MainFrame::OnSearchText, this);
        searchBox->Bind(wxEVT_TEXT_ENTER, &MainFrame::OnSearchEnter, this);
        playButton->Bind(wxEVT_BUTTON, &MainFrame::OnPlay, this);
        pauseButton->Bind(wxEVT_BUTTON, &MainFrame::OnPause, this);
        stopButton->Bind(wxEVT_BUTTON, &MainFrame::OnStop, this);
        listBox->Bind(wxEVT_LISTBOX, &MainFrame::OnSongSelect, this);
        progressBar->Bind(wxEVT_COMMAND_BUTTON_CLICKED, &MainFrame::OnSeek, this);
        refreshButton->Bind(wxEVT_BUTTON, &MainFrame::OnRefresh, this);
        obstructionButton->Bind(wxEVT_BUTTON, &MainFrame::OnObstruction, this);

        // Create timer for progress updates
        progressTimer = new wxTimer(this);
        Bind(wxEVT_TIMER, &MainFrame::OnProgressTimer, this);

        // Set custom font
        wxFont customFont(9, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL,
            wxFONTWEIGHT_NORMAL, false, "Segoe UI");
        listBox->SetFont(customFont);
        playButton->SetFont(customFont);
        pauseButton->SetFont(customFont);
        stopButton->SetFont(customFont);
        timeLabel->SetFont(customFont);
        currentSongLabel->SetFont(customFont);
        refreshButton->SetFont(customFont);
        obstructionButton->SetFont(customFont);

        // Bind the close event
        Bind(wxEVT_CLOSE_WINDOW, &MainFrame::OnClose, this);

        // Initialize button states
        UpdateButtonStates();
    }

    ~MainFrame() {
        StopPlayback();
        delete progressTimer;
    }

    void RefreshAllButtons() {
        // Refresh all buttons in the frame and its children
        RefreshButtonsInWindow(this);
    }

    void OnObstruction(wxCommandEvent& event) {
        if (obstruction) {
            obstruction = false;
            WindowsUtility::showMessageBox("This is not recommended and will cause playback to be substantially lower quality.", "Warning", MB_OK | MB_ICONWARNING);
        }
        else {
            obstruction = true;
        }
        RefreshAllButtons();
        UpdateButtonStates();
    }

    void OnRefresh(wxCommandEvent& event) {
        wxString currentSearch = searchBox->GetValue();
        LoadSongs("songs");
        if (!currentSearch.IsEmpty()) {
            FilterSongs(currentSearch);
        }
        RefreshAllButtons();
    }

private:
    void RefreshButtonsInWindow(wxWindow* window) {
        // Refresh buttons in the current window
        wxWindowList& children = window->GetChildren();
        for (wxWindow* child : children) {
            // Check if the child is a DarkButton
            DarkButton* button = dynamic_cast<DarkButton*>(child);
            if (button) {
                button->ForceRefresh();
            }
            // Recursively check children windows
            RefreshButtonsInWindow(child);
        }
    }

    void CreateSpeedControls(wxWindow* parent) {
        speedSizer = new wxBoxSizer(wxHORIZONTAL);

        std::vector<double> speeds = { 0.5, 0.75, 1.0, 1.25, 1.5 };
        for (size_t i = 0; i < speeds.size(); i++) {
            double speed = speeds[i];
            wxString label = wxString::Format("%.2fx", speed);
            DarkButton* speedBtn = new DarkButton(parent, wxID_ANY, label);

            speedBtn->SetMinSize(wxSize(41, 25));

            if (speed == 1.0) {
                speedBtn->SetBackgroundColour(wxColour(147, 112, 219));
            }

            speedBtn->Bind(wxEVT_BUTTON, [this, speed, speedBtn](wxCommandEvent& event) {
                OnSpeedChange(speed, speedBtn);
                });

            // Add first button
            if (i == 0) {
                speedSizer->Add(speedBtn, 0, wxRIGHT, 5);
            }
            // Add middle buttons
            else if (i < speeds.size() - 1) {
                speedSizer->Add(speedBtn, 0, wxLEFT | wxRIGHT, 5);
            }
            // Add last button
            else {
                speedSizer->Add(speedBtn, 0, wxLEFT, 5);
            }

            speedButtons.push_back(speedBtn);
        }
    }

    void OnSpeedChange(double newSpeed, DarkButton* clickedButton) {
        playbackSpeed = newSpeed;

        // Update button appearances
        for (DarkButton* btn : speedButtons) {
            if (btn == clickedButton) {
                btn->SetBackgroundColour(wxColour(147, 112, 219));
            }
            else {
                btn->SetBackgroundColour(wxColour(60, 60, 60));
            }
            btn->Refresh();
        }
        RefreshAllButtons();
    }

    void OnSeek(wxCommandEvent& event) {
        if (isPlayingSong && totalDuration > 0) {
            int seekPercentage = event.GetInt();
            int seekPosition = (seekPercentage * totalDuration) / 100;

            // Set seeking flag and update progress
            isSeeking = true;
            currentProgress = seekPosition;

            // Update UI immediately
            progressBar->SetValue(seekPercentage);

            int currentSeconds = currentProgress / 1000;
            int totalSeconds = totalDuration / 1000;
            wxString timeStr = wxString::Format("%d:%02d / %d:%02d",
                currentSeconds / 60, currentSeconds % 60,
                totalSeconds / 60, totalSeconds % 60);
            timeLabel->SetLabel(timeStr);
        }
    }


    void FilterSongs(const wxString& searchTerm) {
        listBox->Clear();
        wxString lowerSearchTerm = searchTerm.Lower();

        for (const wxString& song : allSongs) {
            if (song.Lower().Contains(lowerSearchTerm)) {
                listBox->Append(song);
            }
        }
    }

    void OnSearchText(wxCommandEvent& event) {
        FilterSongs(searchBox->GetValue());
    }

    void OnSearchEnter(wxCommandEvent& event) {
        FilterSongs(searchBox->GetValue());

        // If there's exactly one match, select it
        if (listBox->GetCount() == 1) {
            listBox->SetSelection(0);
            wxCommandEvent selEvent(wxEVT_LISTBOX);
            selEvent.SetInt(0);
            OnSongSelect(selEvent);
        }
    }

    void LoadSongs(const std::string& folder) {
        listBox->Clear();
        allSongs.clear();

        try {
            const std::filesystem::path songPath(folder);

            for (const auto& entry : std::filesystem::directory_iterator(songPath)) {
                if (entry.is_regular_file()) {
                    // Convert to wstring then to wxString to handle Unicode properly
                    wxString filename(entry.path().filename().wstring());

                    listBox->Append(filename);
                    allSongs.push_back(filename);
                }
            }
        }
        catch (const std::filesystem::filesystem_error& e) {
            wxMessageBox("Error loading songs", "Error", wxOK | wxICON_ERROR);
        }
    }

    void OnSongSelect(wxCommandEvent& event) {
        // Update selected song filename
        selectedSongFileName = listBox->GetString(listBox->GetSelection()).ToStdString();
        event.Skip();
    }

    std::string GetFullPath(const std::string& filename) {
        return std::filesystem::path("songs").append(filename).string();
    }

    void OnPlay(wxCommandEvent& event) {
        if (listBox->GetSelection() != wxNOT_FOUND) {
            if (isPaused) {
                isPaused = false;
                isPlayingSong = true;
                UpdateButtonStates();
                ResumePlayback();
            }
            else if (!isPlayingSong) {
                wxString filename = listBox->GetString(listBox->GetSelection());
                std::filesystem::path songPath("songs");
                songPath /= filename.ToStdWstring();
                //selectedSongFileName = "songs/" + filename.utf8_string();
                //std::string filePath = songPath.string();
                if (!std::filesystem::exists(songPath)) {
                    wxMessageBox("Cannot find file: " + filename, "Error", wxOK | wxICON_ERROR);
                    return;
                }

                currentSongLabel->SetLabel(filename);
                StopPlayback();
                isPlayingSong = true;
                isPaused = false;
                currentProgress = 0;
                progressTimer->Start(100);

                if (songPlayerThread.joinable()) {
                    songPlayerThread.join();
                }

                songPlayerThread = std::thread(PlaySong, songPath,
                    std::ref(isPlayingSong), std::ref(isPaused),
                    std::ref(currentProgress), std::ref(totalDuration),
                    std::ref(playbackSpeed));

                UpdateButtonStates();
            }
        }
        RefreshAllButtons();
    }

    void OnPause(wxCommandEvent& event) {
        if (isPlayingSong && !isPaused) {
            isPaused = true;
            UpdateButtonStates();
        }
        RefreshAllButtons();
    }

    void OnStop(wxCommandEvent& event) {
        StopPlayback();
        currentProgress = 0;
        progressBar->SetValue(0);
        timeLabel->SetLabel("0:00 / 0:00");
        currentSongLabel->SetLabel("No song selected");
        UpdateButtonStates();
        RefreshAllButtons();
    }

    void StopPlayback() {
        if (isPlayingSong) {
            isPlayingSong = false;
            isPaused = false;
            if (songPlayerThread.joinable()) {
                songPlayerThread.join();
            }
            progressTimer->Stop();
        }
    }

    void ResumePlayback() {
        // Implementation for resuming playback
    }

    // Update the OnProgressTimer function
    void OnProgressTimer(wxTimerEvent& event) {
        if (isPlayingSong && !isPaused && totalDuration > 0) {
            int progressPercent = (currentProgress * 100) / totalDuration;
            progressBar->SetValue(progressPercent);

            // Update time label
            int currentSeconds = currentProgress / 1000;
            int totalSeconds = totalDuration / 1000;
            wxString timeStr = wxString::Format("%d:%02d / %d:%02d",
                currentSeconds / 60, currentSeconds % 60,
                totalSeconds / 60, totalSeconds % 60);
            timeLabel->SetLabel(timeStr);
        }
    }

    void UpdateButtonStates() {
        // Update enabled states
        playButton->Enable(!isPlayingSong || isPaused);
        pauseButton->Enable(isPlayingSong && !isPaused);
        stopButton->Enable(isPlayingSong || isPaused);

        // Update button colors based on current state
        wxColour activeColor(147, 112, 219);    // Purple highlight color
        wxColour inactiveColor(60, 60, 60);     // Regular button color

        if (isPlayingSong && !isPaused) {
            // Playing state
            playButton->SetBackgroundColour(activeColor);
            pauseButton->SetBackgroundColour(inactiveColor);
        }
        else if (isPlayingSong && isPaused) {
            // Paused state
            pauseButton->SetBackgroundColour(activeColor);
            playButton->SetBackgroundColour(inactiveColor);
        }
        else if (obstruction) {
            //If input blocking is disabled
            obstructionButton->SetBackgroundColour(activeColor);
        }
        else {
            // Stopped state
            playButton->SetBackgroundColour(inactiveColor);
            pauseButton->SetBackgroundColour(inactiveColor);
            obstructionButton->SetBackgroundColour(inactiveColor);
        }

        stopButton->SetBackgroundColour(inactiveColor);

        // Force refresh all buttons to ensure proper appearance
        playButton->ForceRefresh();
        pauseButton->ForceRefresh();
        stopButton->ForceRefresh();
        obstructionButton->ForceRefresh();
    }

    void OnClose(wxCloseEvent& event) {
        // Ensure clean shutdown
        StopPlayback();
        event.Skip();
    }
    DarkListBox* listBox;
    DarkButton* playButton;
    DarkButton* pauseButton;
    DarkButton* stopButton;
    DarkButton* refreshButton;
    DarkButton* obstructionButton;
    CustomProgressBar* progressBar;
    wxStaticText* timeLabel;
    wxTimer* progressTimer;
    wxStaticText* currentSongLabel;
    wxBoxSizer* speedSizer;
    std::vector<DarkButton*> speedButtons;
    DarkSearchBox* searchBox;
    std::vector<wxString> allSongs;

    std::thread songPlayerThread;
    bool isPlayingSong;
    bool isPaused;
    bool isSeeking;
    std::string selectedSongFileName;
    int currentProgress;
    int totalDuration;
    double playbackSpeed = 1.0;

};

class MusicPlayerApp : public wxApp {
public:
    bool OnInit() {
        // Enable dark mode for the entire application (Windows)
#ifdef __WXMSW__
        typedef BOOL(WINAPI* SetPreferredAppMode)(INT);
        HMODULE hUxtheme = LoadLibraryA("uxtheme.dll");
        if (hUxtheme) {
            SetPreferredAppMode SetAppMode =
                (SetPreferredAppMode)GetProcAddress(hUxtheme, MAKEINTRESOURCEA(135));
            if (SetAppMode) {
                SetAppMode(2); // 2 = Force Dark Mode
            }
            FreeLibrary(hUxtheme);
        }
#endif

        MainFrame* frame = new MainFrame();
        frame->Show(true);
        return true;
    }
};
wxIMPLEMENT_APP(MusicPlayerApp);