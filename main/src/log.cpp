#include <array>
#include <algorithm>

#include <driver/gpio.h>

#include <soccerbot/log.h>
#include <soccerbot/mutex.h>

namespace soccerbot {

static vprintf_like_t originalLogFun = nullptr;

#define MAX_LOG_FUNS 2
static std::array<LogSinkFun, MAX_LOG_FUNS> customLogFuns;
static int customLogFunsCount = 0;

void addLogSink(LogSinkFun fun) {
    if (std::find(customLogFuns.begin(), customLogFuns.end(), fun) != customLogFuns.end()) {
        return;
    }

    customLogFuns[customLogFunsCount++] = fun;
}

void removelogSink(LogSinkFun fun) {
    auto newEnd = std::remove(customLogFuns.begin(), customLogFuns.end(), fun);
    customLogFunsCount = newEnd - customLogFuns.begin();
}

static bool inWifiBugLog = false;

static char logBuf[MAX_LOG_SIZE + 1];
static std::optional<Mutex> logMutex;

int customLogVprintf(const char* format, va_list args) {
    if (xPortInIsrContext()) {
        printf("IN ISR!!\n");
    }

    auto lock = logMutex->take();

    int len = vsnprintf(logBuf, MAX_LOG_SIZE, format, args);
    std::string_view str(logBuf, len);
    
    if (str.size() == 0) {
        printf("Empty log for fstring %s\n", format);
        return len;
    }

    // Ignore "W (num) wifi:exceed max band ..." errors
    // Unfortunately this ends up ignoring all wifi warnings. When we update ESP-IDF this bug will go away.
    // For some reason, the message is split at the colon and the newline
    if (str.find("W (") != std::string_view::npos && str.find(") wifi:") != std::string_view::npos) {
        return len;
    }

    if (str.find("exceed") != std::string_view::npos) {
        inWifiBugLog = true;
        return len;
    }

    if (str == "\n" && inWifiBugLog) {
        inWifiBugLog = false;
        return len;
    }

    // Log to USB/UART, freeing va_list
    originalLogFun(format, args);

    // Ignore network stack errors when logging to the network
    if (str.find("rnet") != std::string_view::npos) {
        return len;
    }

    for (LogSinkFun* i = customLogFuns.data(); i < customLogFuns.data() + customLogFunsCount; i++) {
        (*i)(str);
    }

    return len;
}

void overrideLog() {
    if (!logMutex.has_value()) {
        logMutex.emplace();
    }

    auto lock = logMutex->take();
    originalLogFun = esp_log_set_vprintf(customLogVprintf);
}

};