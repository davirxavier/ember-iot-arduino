//
// Created by xav on 3/27/25.
//

#ifndef FIREBASE_UTIL_H
#define FIREBASE_UTIL_H

namespace FirePropUtil {
    inline size_t countOccurrences(const char *str, const char *sub) {
        int count = 0;
        const char *pos = str;
        int subLen = strlen(sub);

        if (subLen == 0) return 0; // Avoid infinite loop for empty substring

        while ((pos = strstr(pos, sub)) != nullptr) {
            count++;
            pos += subLen; // Move past the last found occurrence
        }

        return count;
    }

    inline void replaceSubstring(char *str, const char *oldWord, const char *newWord) {
        char *pos, *temp = str;
        size_t oldLen = strlen(oldWord);
        size_t newLen = strlen(newWord);

        size_t oldCount = countOccurrences(str, oldWord);
        char buffer[strlen(str) - (oldCount * oldLen) + (oldCount * newLen) + 16];
        buffer[0] = '\0';

        while ((pos = strstr(temp, oldWord)) != nullptr) {
            // Copy characters before the found substring
            strncat(buffer, temp, pos - temp);
            // Append the replacement string
            strcat(buffer, newWord);
            // Move past the old substring
            temp = pos + oldLen;
        }

        strcat(buffer, temp); // Append remaining part
        strcpy(str, buffer);  // Copy modified string back
    }

    inline bool endsWith(const char *str, const char *suffix)
    {
        if (!str || !suffix)
        {
            return true;
        }

        size_t lenstr = strlen(str);
        size_t lensuffix = strlen(suffix);

        if (lensuffix > lenstr)
        {
            return false;
        }

        return strncmp(str + lenstr - lensuffix, suffix, lensuffix) == 0;
    }

    inline void setToNull(void **arr, size_t length)
    {
        for (size_t i = 0; i < length; i++) // No braces
            arr[i] = nullptr;
    }

    inline uint32_t fnv1aHash(const char* str) {
        uint32_t hash = 2166136261UL;  // FNV offset basis

        if (str == nullptr)
        {
            return hash;
        }

        while (*str) {
            hash ^= (uint8_t)(*str++);
            hash *= 16777619UL;  // FNV prime
        }
        return hash;
    }
}

#endif //FIREBASE_UTIL_H
