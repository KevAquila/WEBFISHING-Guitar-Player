#pragma once
// I'm too lazy to make a proper class.
#include <array>
#include <vector>
#include <string>
#include <fstream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <Windows.h>
#include <Psapi.h>
#include <tlhelp32.h>
#include <iostream>
#include <chrono>
#include <thread>
#include <algorithm>
#include "WindowsUtility.h"
#include "InputBlocker.h"
#include "globals.h"



#include "MidiProcessing.h"

const std::array<int, 16> low_e = { 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55 };
const std::array<int, 16> a = { 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60 };
const std::array<int, 16> d = { 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65 };
const std::array<int, 16> g = { 55, 56, 57, 58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70 };
const std::array<int, 16> b = { 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74 };
const std::array<int, 16> high_e = { 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79 };
HWND targetWindow = NULL;
static std::vector<int> lastClickedPositions(6, 0);  // Track last clicked position for each string





struct MouseClick {
    int x;
    int y;
    bool isLeft;
};

std::pair<int, int> getClickPosition(int windowWidth, int windowHeight, int x, int y) {
    const double xMinRatio = 0.17740;
    const double xMaxRatio = 0.263975;
    const double yMinRatio = 0.070;
    const double yMaxRatio = 0.916014;

    int gridWidth = static_cast<int>((xMaxRatio - xMinRatio) * windowWidth);
    int gridHeight = static_cast<int>(((yMaxRatio - yMinRatio) * windowHeight));

    double cellWidth = static_cast<double>(gridWidth) / 5;
    double cellHeight = static_cast<double>(gridHeight) / 15;

    int offsetX = static_cast<int>(xMinRatio * windowWidth);
    int offsetY = static_cast<int>(yMinRatio * windowHeight);

    int clickX = offsetX + static_cast<int>((x * cellWidth) + (cellWidth / 2));
    int clickY = offsetY + static_cast<int>((y * cellHeight) + (cellHeight / 2));

    return { clickX, clickY };
}

void sendMultipleKeys(const std::vector<char>& keys, int sleepTime, bool down = false) {
    if (sleepTime != 0 || down == true)
    {
        for (const auto& keyPress : keys) {
            WORD vkCode = VkKeyScanA(keyPress);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

            PostMessage(targetWindow, WM_KEYDOWN, vkCode, (scanCode << 16) | 1);
        }
    }

    if (sleepTime != 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));

    if (down == false) {
        for (const auto& keyPress : keys) {
            WORD vkCode = VkKeyScanA(keyPress);
            WORD scanCode = MapVirtualKeyA(vkCode, MAPVK_VK_TO_VSC);

            PostMessage(targetWindow, WM_KEYUP, vkCode, (scanCode << 16) | 0xC0000001);
        }
    }
}

void sendMultipleClicks(const std::vector<MouseClick>& clicks, int sleepTime, bool down = false) {
    for (const auto& click : clicks) {
        LPARAM lParam = MAKELPARAM(click.x, click.y);
        WPARAM wParam = click.isLeft ? MK_LBUTTON : MK_RBUTTON;
        UINT downMsg = click.isLeft ? WM_LBUTTONDOWN : WM_RBUTTONDOWN;
        UINT upMsg = click.isLeft ? WM_LBUTTONUP : WM_RBUTTONUP;

        PostMessage(targetWindow, downMsg, wParam, lParam);


        std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));
        lParam = MAKELPARAM(click.x, click.y);
        wParam = click.isLeft ? MK_LBUTTON : MK_RBUTTON;
        upMsg = click.isLeft ? WM_LBUTTONUP : WM_RBUTTONUP;

        PostMessage(targetWindow, upMsg, wParam, lParam);
    }
}

std::vector<std::string> explode_midi(std::string const& s, char delim)
{
    std::vector<std::string> result;
    std::istringstream iss(s);

    for (std::string token; std::getline(iss, token, delim); )
    {
        result.push_back(std::move(token));
    }

    return result;
}

void LoadSongFromFile(const std::filesystem::path& filepath, std::vector<SoundEvent>& song) {
    song.clear();
    std::string fileContent;

    // Handle MIDI files
    if (filepath.extension().string() == ".mid") {
        fileContent = convertMidiToText(filepath);
    }
    // Handle regular text files
    else {
        std::ifstream songFile(filepath);
        if (!songFile.is_open()) {
            WindowsUtility::showMessageBox(
                "Failed to open song file: " + filepath.string(),
                "Error",
                MB_OK | MB_ICONERROR
            );
            return;
        }

        // Read entire file into string
        std::stringstream buffer;
        buffer << songFile.rdbuf();
        fileContent = buffer.str();
    }

    // Process the content string
    std::istringstream contentStream(fileContent);
    std::string tempLine;
    int outOfRangeNotes = 0;

    while (std::getline(contentStream, tempLine)) {
        if (tempLine.size() < 3)
            continue;

        std::vector<std::string> split = explode_midi(tempLine, ' ');
        if (split.empty())
            continue;

        try {
            int time = std::stoi(split[0]);
            std::vector<uint8_t> tempNotes;

            for (size_t i = 1; i < split.size(); i++) {
                float note = std::stof(split[i]);
                tempNotes.push_back(static_cast<uint8_t>(note));
                if (note < 40 || note > 79) {
                    outOfRangeNotes++;
                }
            }

            song.push_back(SoundEvent(time, tempNotes));
        }
        catch (const std::exception& e) {
            std::cerr << "Error processing line: " << tempLine << "\nError: " << e.what() << std::endl;
            continue;
        }
    }

    std::cout << "Song loading complete." << std::endl;
    if (outOfRangeNotes > 0) {
        std::cout << "Number of notes out of range (< 40 or > 79): " << outOfRangeNotes << std::endl;
    }
}

std::array<std::chrono::steady_clock::time_point, 6> lastStringUsageTime;
void playNotes(const std::vector<uint8_t>& notes) {
    std::vector<uint8_t> sortedNotes = notes;
    std::sort(sortedNotes.begin(), sortedNotes.end(), std::greater<float>());
    std::vector<MouseClick> clicksToSend;
    std::vector<char> keysToPress;
    std::vector<bool> stringUsed(6, false);

    RECT windowRect;
    GetClientRect(targetWindow, &windowRect);
    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    auto currentTime = std::chrono::steady_clock::now();

    // Helper struct to store possible positions for a note
    struct StringPosition {
        int stringIndex;
        int fretPosition;
        bool requiresPositionChange;
    };

    // Helper function to find all possible positions for a note
    auto findAllPositions = [&](int intNote) -> std::vector<StringPosition> {
        std::vector<StringPosition> positions;

        // Array of string data for iteration
        const std::array<std::pair<const std::array<int, 16>*, int>, 6> strings = {
            std::make_pair(&high_e, 5),
            std::make_pair(&b, 4),
            std::make_pair(&g, 3),
            std::make_pair(&d, 2),
            std::make_pair(&a, 1),
            std::make_pair(&low_e, 0)
        };

        for (const auto& [stringNotes, stringIndex] : strings) {
            if (stringUsed[stringIndex]) continue;

            auto it = std::find(stringNotes->begin(), stringNotes->end(), intNote);
            if (it != stringNotes->end()) {
                int fretPos = static_cast<int>(std::distance(stringNotes->begin(), it));
                bool requiresChange = (lastClickedPositions[stringIndex] != fretPos);

                positions.push_back({
                    stringIndex,
                    fretPos,
                    requiresChange
                    });
            }
        }
        return positions;
        };

    // Process each note
    for (float note : sortedNotes) {
        int intNote = static_cast<int>(std::round(note));
        auto positions = findAllPositions(intNote);

        if (!positions.empty()) {
            // First, try to find a position that doesn't require a change
            auto noChangeIt = std::find_if(positions.begin(), positions.end(),
                [](const StringPosition& pos) { return !pos.requiresPositionChange; });

            // If no position without change is found, use the first available position
            const auto& bestPos = (noChangeIt != positions.end()) ? *noChangeIt : positions[0];
            int stringIndex = bestPos.stringIndex;

            // Update tracking variables
            if (bestPos.requiresPositionChange) {
                auto [clickX, clickY] = getClickPosition(windowWidth, windowHeight, stringIndex, bestPos.fretPosition);
                clicksToSend.push_back({ clickX, clickY, true });
                lastClickedPositions[stringIndex] = bestPos.fretPosition;
            }

            // Add appropriate key press
            char key;
            switch (stringIndex) {
            case 0: key = 'q'; break;
            case 1: key = 'w'; break;
            case 2: key = 'e'; break;
            case 3: key = 'r'; break;
            case 4: key = 't'; break;
            case 5: key = 'y'; break;
            default: continue;
            }
            keysToPress.push_back(key);
            stringUsed[stringIndex] = true;
            lastStringUsageTime[stringIndex] = currentTime;
        }
        else {
            std::cout << "  No match found for note " << note << " (int: " << intNote << ")" << std::endl;
        }
    }

    // Send all accumulated clicks and key presses
    if (!clicksToSend.empty()) {
        sendMultipleClicks(clicksToSend, 1, true);
    }
    int extraSleep = 6 - clicksToSend.size();
    if (extraSleep > 0)
        std::this_thread::sleep_for(std::chrono::milliseconds(extraSleep));

    if (!keysToPress.empty()) {
        sendMultipleKeys(keysToPress, 12);
    }
}

void clickThroughAllPositions(int windowWidth, int windowHeight) {
    for (int string = 0; string <= 5; ++string) {
        for (int fret = 0; fret <= 15; ++fret) {
            auto [clickX, clickY] = getClickPosition(windowWidth, windowHeight, string, fret);

            std::vector<MouseClick> clicksToSend;
            clicksToSend.push_back({ clickX, clickY, true });
            sendMultipleClicks(clicksToSend, 1, true);

            std::this_thread::sleep_for(std::chrono::milliseconds(200));

            std::cout << "Clicked position: string " << string << ", fret " << fret
                << " (X: " << clickX << ", Y: " << clickY << ")" << std::endl;
        }
    }
}

void PlaySong(const std::filesystem::path& songPath, bool& isPlaying, bool& isPaused, int& currentProgress, int& totalDuration, double& playbackSpeed) {
    std::vector<SoundEvent> songData;
    LoadSongFromFile(songPath, songData);

    if (!songData.empty()) {
        totalDuration = songData.back().timestamp;
    }

    const std::string processName = "webfishing.exe";
    targetWindow = WindowsUtility::findWindowByProcessName(processName);
    if (!WindowsUtility::isWindowValid(targetWindow)) {
        WindowsUtility::showMessageBox("Failed to find target window.", "Error", MB_OK | MB_ICONERROR);
        return;
    }

    if (WindowsUtility::needsElevation(targetWindow)) {
        WindowsUtility::elevateWithPrompt(
            "Webfishing is running with elevated privileges.\n"
            "Guitar Player needs to restart with administrator rights to be able to play.");
        return;
    }

    auto [windowWidth, windowHeight] = WindowsUtility::getWindowDimensions(targetWindow);
    auto [clickX, clickY] = getClickPosition(windowWidth, windowHeight, 5, 0);
    auto [clickX2, clickY2] = getClickPosition(windowWidth, windowHeight, 4, 0);
    clickX += (clickX - clickX2);
    std::vector<MouseClick> clicksToSend;
    clicksToSend.push_back({ clickX, clickY, true });
    sendMultipleClicks(clicksToSend, 1, true); // Reset Webfishing guitar
    for (unsigned i = 0; i < 6; i++) { // Reset the clicked positions.
        lastClickedPositions[i] = 0;
    }
    Sleep(300);

    int previousProgress = currentProgress;
    double accumulatedTime = currentProgress;
    auto lastUpdateTime = std::chrono::high_resolution_clock::now();
    InputBlocker inputBlocker;

    if (obstruction) {
        inputBlocker.Start(targetWindow); 
    }
    while (isPlaying) {
        // Handle pausing
        if (isPaused) {
            inputBlocker.Pause();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            lastUpdateTime = std::chrono::high_resolution_clock::now();
            continue;
        }
        else {
            inputBlocker.Resume();
        }

        auto now = std::chrono::high_resolution_clock::now();
        auto realTimeDelta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();

        // Check for seek operation
        if (abs(currentProgress - previousProgress) > 100) {
            accumulatedTime = currentProgress;
            lastUpdateTime = now;
        }
        else {
            accumulatedTime += realTimeDelta * playbackSpeed;
            currentProgress = static_cast<int>(accumulatedTime);
        }

        previousProgress = currentProgress;
        lastUpdateTime = now;

        // Find the next note to play
        size_t nextIndex = 0;
        for (; nextIndex < songData.size(); nextIndex++) {
            if (songData[nextIndex].timestamp >= currentProgress) {
                break;
            }
        }

        if (nextIndex >= songData.size()) {
            isPlaying = false;
            break;
        }

        const auto& nextEvent = songData[nextIndex];
        long long waitTime = static_cast<long long>((nextEvent.timestamp - currentProgress) / playbackSpeed);

        if (waitTime > 0) {
            // Wait in small intervals to stay responsive
            const long long checkInterval = 50;
            const int seekThreshold = 100;
            while (waitTime > 0 && isPlaying && !isPaused) {
                auto sleepTime = std::min(waitTime, checkInterval);
                std::this_thread::sleep_for(std::chrono::milliseconds(sleepTime));

                now = std::chrono::high_resolution_clock::now();
                realTimeDelta = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastUpdateTime).count();

                // Check for seek during wait
                if (abs(currentProgress - previousProgress) > seekThreshold) {
                    // Update timing to match seek position
                    accumulatedTime = currentProgress;
                    lastUpdateTime = now;
                    break;
                }

                accumulatedTime += realTimeDelta * playbackSpeed;
                currentProgress = static_cast<int>(accumulatedTime);
                lastUpdateTime = now;
                previousProgress = currentProgress;

                waitTime = static_cast<long long>((nextEvent.timestamp - currentProgress) / playbackSpeed);
            }
        }

        if (isPlaying && !isPaused && abs(currentProgress - nextEvent.timestamp) < 100) {
            playNotes(nextEvent.notes);
        }
    }
    inputBlocker.Stop();
    isPlaying = false;
}