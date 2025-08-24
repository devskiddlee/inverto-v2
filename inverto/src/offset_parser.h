#define CURL_STATICLIB
#include <curl/curl.h>

#include <iostream>
#include <format>
#include <fstream>
#include <list>
#include <sstream>
#include <algorithm>
#include <vector>
#include <chrono>
#include <map>
#include <thread>
#include <unordered_map>
#include <cstddef>
#include <stdexcept>
#include <filesystem>
#include <Windows.h>
#include <string_view>

class OP {
public:
    inline static std::string offset_parse_operation_update = "";
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
	((std::string*)userp)->append((char*)contents, size * nmemb);
	return size * nmemb;
}

std::string getGithubFileContent(const std::string& url) {
	CURL* curl;
	CURLcode res;
	std::string readBuffer;

	curl = curl_easy_init();
	if (!curl) {
		throw std::runtime_error("Failed to initialize CURL");
	}

	curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
	curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);

	curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

	res = curl_easy_perform(curl);
	if (res != CURLE_OK) {
		curl_easy_cleanup(curl);
		throw std::runtime_error(curl_easy_strerror(res));
	}

	curl_easy_cleanup(curl);
	return readBuffer;
}

int Find(const std::string& data, int startIndex, const std::vector<std::string>& searchTerms, char endChar = '\0')
{
    int offset = startIndex;
    int index = 0;
    std::list<int> done;
    for (std::string mask : searchTerms) {
        std::string text = data.substr(offset);
        int c = 0;
        int currindex = offset;
        for (char ch : text) {
            if (ch == endChar)
                return -1;

            if (ch == mask[c])
                c++;
            else
                c = 0;

            if (c == mask.size())
            {
                offset = currindex - (int)mask.size() + 1;
                done.push_back(offset);
                break;
            }

            currindex++;
        }
        index++;
    }

    if (done.size() != searchTerms.size())
        return -1;

    return offset;
}

std::string SafeSubstring(std::string text, int start, int length)
{
    return text.length() <= start ? ""
        : text.length() - start <= length ? text.substr(start)
        : text.substr(start, length);
}

std::string parseOffsets()
{
    std::vector<std::string> offsets = { "offsets.cs", "client_dll.cs" };
    std::vector<std::string> offset_strings;

    for (const auto& os : offsets)
    {
        offset_strings.push_back(getGithubFileContent("https://raw.githubusercontent.com/a2x/cs2-dumper/refs/heads/main/output/" + os));
    }

    std::vector<std::string> operations;
    size_t index = 0;

    std::string parsed_offsets;
    int done = 0;

    for (const auto& data : offset_strings)
    {
        std::string baseName = offsets[index].substr(offsets[index].find_last_of("\\/") + 1);
        baseName = baseName.substr(0, baseName.find_first_of("."));

        if (baseName == "offsets")
        {
            int i = 0;
            std::string s;
            while (true)
            {
                i = Find(data, i + 6, { " nint " }, '}');
                if (i < 0) break;

                std::string name = SafeSubstring(data, i + 6, 100);
                name = name.substr(0, name.find_first_of(" "));

                int valIndex = Find(data, 0, { "ClientDll", name, "0x" });
                if (valIndex < 0) break;

                std::string value = SafeSubstring(data, valIndex, 15);
                value = value.substr(0, value.find_first_of(";"));

                s += name + "->" + value + ";";
                operations.push_back(name + "->" + value);
                done += 1;

                std::ostringstream stream;
                stream << "Parsing offsets... [" << done << " done]";
                OP::offset_parse_operation_update = stream.str();
            }

            parsed_offsets.append(s);
        }

        if (baseName == "client_dll")
        {
            int text = Find(data, 0, { "ClientDll" });
            int i = text + (int)std::string("ClientDll").length();

            std::vector<std::string> classes;
            std::vector<int> clI;

            while (true)
            {
                i = Find(data, i + (int)std::string("public static class ").length(), { "public static class " });
                if (i < 0) break;

                std::string name = SafeSubstring(data, i + (int)std::string("public static class ").length(), 100);
                name = name.substr(0, name.find_first_of(" "));

                classes.push_back(name);
                clI.push_back(i);
            }

            std::string s;
            for (const auto& cl : classes)
            {
                i = Find(data, i + 6, { "public static class " + cl + " " });
                while (true)
                {
                    i = Find(data, i + 6, { " nint " }, '}');
                    if (i < 0) break;

                    std::string name = SafeSubstring(data, i + 6, 100);
                    name = name.substr(0, name.find_first_of(" "));

                    int valIndex = Find(data, i + 6, { "0x" });
                    if (valIndex < 0) break;

                    std::string value = SafeSubstring(data, valIndex, 15);
                    value = value.substr(0, value.find_first_of(";"));

                    s += cl + "->" + name + "->" + value + ";";
                    operations.push_back(cl + "->" + name + "->" + value);
                    done += 1;

                    std::ostringstream stream;
                    stream << "Parsing offsets... [" << done << " done]";
                    OP::offset_parse_operation_update = stream.str();
                }
            }

            parsed_offsets.append(s);
        }

        index++;
    }

    return parsed_offsets;
}