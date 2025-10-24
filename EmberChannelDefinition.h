/******************************************************************************
* Project Name: EmberIoT
*
* Ember IoT is a simple proof of concept for a Firebase-hosted IoT
* cloud designed to work with Arduino-based devices and an Android mobile app.
* It enables microcontrollers to connect to the cloud, sync data,
* and interact with a mobile interface using Firebase Authentication and
* Firebase Realtime Database services. This project simplifies creating IoT
* infrastructure without the need for a dedicated server.
*
* Copyright (c) 2025 davirxavier
*
* MIT License
*
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*****************************************************************************/

#ifndef EMBERCHANNELDEF_H
#define EMBERCHANNELDEF_H

class EmberIotProp
{
public:
    explicit EmberIotProp(const char* data, const bool hasChanged) : hasChanged(hasChanged), data(data)
    {
    }

    int toInt() const
    {
        return data != nullptr ? atoi(data) : 0;
    }

    long toLong() const
    {
        return data != nullptr ? atol(data) : 0l;
    }

    long long toLongLong() const
    {
        return data != nullptr ? atoll(data) : 0ll;
    }

    double toDouble() const
    {
        return data != nullptr ? atof(data) : 0.0;
    }

    const char* toString() const
    {
        return this->data;
    }

    /**
     * True if the value itself has changed from the last emitted value.
     */
    const bool hasChanged;

private:
    const char* data;
};

typedef void (*EmberIotUpdateCallback)(const EmberIotProp& p);

#if defined(__GNUC__)
#define EMBER_ATTR_WEAK __attribute__((weak))
#else
#define EMBER_ATTR_WEAK
#warning "Weak attribute not supported; symbol overriding may not work. You will need to define callbacks for every channel < EMBER_CHANNEL_COUNT."
#endif

#if EMBER_CHANNEL_COUNT > 0
void ember_channel_cb_0(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_0
#endif

#if EMBER_CHANNEL_COUNT > 1
void ember_channel_cb_1(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_1
#endif

#if EMBER_CHANNEL_COUNT > 2
void ember_channel_cb_2(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_2
#endif

#if EMBER_CHANNEL_COUNT > 3
void ember_channel_cb_3(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_3
#endif

#if EMBER_CHANNEL_COUNT > 4
void ember_channel_cb_4(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_4
#endif

#if EMBER_CHANNEL_COUNT > 5
void ember_channel_cb_5(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_5
#endif

#if EMBER_CHANNEL_COUNT > 6
void ember_channel_cb_6(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_6
#endif

#if EMBER_CHANNEL_COUNT > 7
void ember_channel_cb_7(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_7
#endif

#if EMBER_CHANNEL_COUNT > 8
void ember_channel_cb_8(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_8
#endif

#if EMBER_CHANNEL_COUNT > 9
void ember_channel_cb_9(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_9
#endif

#if EMBER_CHANNEL_COUNT > 10
void ember_channel_cb_10(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_10
#endif

#if EMBER_CHANNEL_COUNT > 11
void ember_channel_cb_11(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_11
#endif

#if EMBER_CHANNEL_COUNT > 12
void ember_channel_cb_12(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_12
#endif

#if EMBER_CHANNEL_COUNT > 13
void ember_channel_cb_13(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_13
#endif

#if EMBER_CHANNEL_COUNT > 14
void ember_channel_cb_14(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_14
#endif

#if EMBER_CHANNEL_COUNT > 15
void ember_channel_cb_15(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_15
#endif

#if EMBER_CHANNEL_COUNT > 16
void ember_channel_cb_16(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_16
#endif

#if EMBER_CHANNEL_COUNT > 17
void ember_channel_cb_17(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_17
#endif

#if EMBER_CHANNEL_COUNT > 18
void ember_channel_cb_18(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_18
#endif

#if EMBER_CHANNEL_COUNT > 19
void ember_channel_cb_19(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_19
#endif

#if EMBER_CHANNEL_COUNT > 20
void ember_channel_cb_20(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_20
#endif

#if EMBER_CHANNEL_COUNT > 21
void ember_channel_cb_21(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_21
#endif

#if EMBER_CHANNEL_COUNT > 22
void ember_channel_cb_22(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_22
#endif

#if EMBER_CHANNEL_COUNT > 23
void ember_channel_cb_23(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_23
#endif

#if EMBER_CHANNEL_COUNT > 24
void ember_channel_cb_24(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_24
#endif

#if EMBER_CHANNEL_COUNT > 25
void ember_channel_cb_25(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_25
#endif

#if EMBER_CHANNEL_COUNT > 26
void ember_channel_cb_26(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_26
#endif

#if EMBER_CHANNEL_COUNT > 27
void ember_channel_cb_27(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_27
#endif

#if EMBER_CHANNEL_COUNT > 28
void ember_channel_cb_28(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_28
#endif

#if EMBER_CHANNEL_COUNT > 29
void ember_channel_cb_29(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_29
#endif

#if EMBER_CHANNEL_COUNT > 30
void ember_channel_cb_30(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_30
#endif

#if EMBER_CHANNEL_COUNT > 31
void ember_channel_cb_31(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_31
#endif

#if EMBER_CHANNEL_COUNT > 32
void ember_channel_cb_32(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_32
#endif

#if EMBER_CHANNEL_COUNT > 33
void ember_channel_cb_33(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_33
#endif

#if EMBER_CHANNEL_COUNT > 34
void ember_channel_cb_34(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_34
#endif

#if EMBER_CHANNEL_COUNT > 35
void ember_channel_cb_35(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_35
#endif

#if EMBER_CHANNEL_COUNT > 36
void ember_channel_cb_36(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_36
#endif

#if EMBER_CHANNEL_COUNT > 37
void ember_channel_cb_37(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_37
#endif

#if EMBER_CHANNEL_COUNT > 38
void ember_channel_cb_38(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_38
#endif

#if EMBER_CHANNEL_COUNT > 39
void ember_channel_cb_39(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_39
#endif

#if EMBER_CHANNEL_COUNT > 40
void ember_channel_cb_40(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_40
#endif

#if EMBER_CHANNEL_COUNT > 41
void ember_channel_cb_41(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_41
#endif

#if EMBER_CHANNEL_COUNT > 42
void ember_channel_cb_42(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_42
#endif

#if EMBER_CHANNEL_COUNT > 43
void ember_channel_cb_43(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_43
#endif

#if EMBER_CHANNEL_COUNT > 44
void ember_channel_cb_44(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_44
#endif

#if EMBER_CHANNEL_COUNT > 45
void ember_channel_cb_45(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_45
#endif

#if EMBER_CHANNEL_COUNT > 46
void ember_channel_cb_46(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_46
#endif

#if EMBER_CHANNEL_COUNT > 47
void ember_channel_cb_47(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_47
#endif

#if EMBER_CHANNEL_COUNT > 48
void ember_channel_cb_48(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_48
#endif

#if EMBER_CHANNEL_COUNT > 49
void ember_channel_cb_49(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_49
#endif

#if EMBER_CHANNEL_COUNT > 50
void ember_channel_cb_50(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_50
#endif

#if EMBER_CHANNEL_COUNT > 51
void ember_channel_cb_51(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_51
#endif

#if EMBER_CHANNEL_COUNT > 52
void ember_channel_cb_52(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_52
#endif

#if EMBER_CHANNEL_COUNT > 53
void ember_channel_cb_53(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_53
#endif

#if EMBER_CHANNEL_COUNT > 54
void ember_channel_cb_54(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_54
#endif

#if EMBER_CHANNEL_COUNT > 55
void ember_channel_cb_55(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_55
#endif

#if EMBER_CHANNEL_COUNT > 56
void ember_channel_cb_56(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_56
#endif

#if EMBER_CHANNEL_COUNT > 57
void ember_channel_cb_57(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_57
#endif

#if EMBER_CHANNEL_COUNT > 58
void ember_channel_cb_58(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_58
#endif

#if EMBER_CHANNEL_COUNT > 59
void ember_channel_cb_59(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_59
#endif

#if EMBER_CHANNEL_COUNT > 60
void ember_channel_cb_60(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_60
#endif

#if EMBER_CHANNEL_COUNT > 61
void ember_channel_cb_61(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_61
#endif

#if EMBER_CHANNEL_COUNT > 62
void ember_channel_cb_62(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_62
#endif

#if EMBER_CHANNEL_COUNT > 63
void ember_channel_cb_63(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_63
#endif

#if EMBER_CHANNEL_COUNT > 64
void ember_channel_cb_64(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_64
#endif

#if EMBER_CHANNEL_COUNT > 65
void ember_channel_cb_65(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_65
#endif

#if EMBER_CHANNEL_COUNT > 66
void ember_channel_cb_66(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_66
#endif

#if EMBER_CHANNEL_COUNT > 67
void ember_channel_cb_67(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_67
#endif

#if EMBER_CHANNEL_COUNT > 68
void ember_channel_cb_68(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_68
#endif

#if EMBER_CHANNEL_COUNT > 69
void ember_channel_cb_69(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_69
#endif

#if EMBER_CHANNEL_COUNT > 70
void ember_channel_cb_70(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_70
#endif

#if EMBER_CHANNEL_COUNT > 71
void ember_channel_cb_71(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_71
#endif

#if EMBER_CHANNEL_COUNT > 72
void ember_channel_cb_72(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_72
#endif

#if EMBER_CHANNEL_COUNT > 73
void ember_channel_cb_73(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_73
#endif

#if EMBER_CHANNEL_COUNT > 74
void ember_channel_cb_74(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_74
#endif

#if EMBER_CHANNEL_COUNT > 75
void ember_channel_cb_75(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_75
#endif

#if EMBER_CHANNEL_COUNT > 76
void ember_channel_cb_76(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_76
#endif

#if EMBER_CHANNEL_COUNT > 77
void ember_channel_cb_77(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_77
#endif

#if EMBER_CHANNEL_COUNT > 78
void ember_channel_cb_78(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_78
#endif

#if EMBER_CHANNEL_COUNT > 79
void ember_channel_cb_79(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_79
#endif

#if EMBER_CHANNEL_COUNT > 80
void ember_channel_cb_80(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_80
#endif

#if EMBER_CHANNEL_COUNT > 81
void ember_channel_cb_81(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_81
#endif

#if EMBER_CHANNEL_COUNT > 82
void ember_channel_cb_82(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_82
#endif

#if EMBER_CHANNEL_COUNT > 83
void ember_channel_cb_83(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_83
#endif

#if EMBER_CHANNEL_COUNT > 84
void ember_channel_cb_84(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_84
#endif

#if EMBER_CHANNEL_COUNT > 85
void ember_channel_cb_85(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_85
#endif

#if EMBER_CHANNEL_COUNT > 86
void ember_channel_cb_86(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_86
#endif

#if EMBER_CHANNEL_COUNT > 87
void ember_channel_cb_87(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_87
#endif

#if EMBER_CHANNEL_COUNT > 88
void ember_channel_cb_88(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_88
#endif

#if EMBER_CHANNEL_COUNT > 89
void ember_channel_cb_89(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_89
#endif

#if EMBER_CHANNEL_COUNT > 90
void ember_channel_cb_90(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_90
#endif

#if EMBER_CHANNEL_COUNT > 91
void ember_channel_cb_91(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_91
#endif

#if EMBER_CHANNEL_COUNT > 92
void ember_channel_cb_92(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_92
#endif

#if EMBER_CHANNEL_COUNT > 93
void ember_channel_cb_93(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_93
#endif

#if EMBER_CHANNEL_COUNT > 94
void ember_channel_cb_94(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_94
#endif

#if EMBER_CHANNEL_COUNT > 95
void ember_channel_cb_95(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_95
#endif

#if EMBER_CHANNEL_COUNT > 96
void ember_channel_cb_96(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_96
#endif

#if EMBER_CHANNEL_COUNT > 97
void ember_channel_cb_97(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_97
#endif

#if EMBER_CHANNEL_COUNT > 98
void ember_channel_cb_98(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_98
#endif

#if EMBER_CHANNEL_COUNT > 99
void ember_channel_cb_99(const EmberIotProp& prop) EMBER_ATTR_WEAK;
#else
#define EMBER_CHANNEL_DEFINE_STUB_99
#endif

EmberIotUpdateCallback callbacks[EMBER_CHANNEL_COUNT] = {
#if EMBER_CHANNEL_COUNT > 0
    ember_channel_cb_0,
#endif
#if EMBER_CHANNEL_COUNT > 1
    ember_channel_cb_1,
#endif
#if EMBER_CHANNEL_COUNT > 2
    ember_channel_cb_2,
#endif
#if EMBER_CHANNEL_COUNT > 3
    ember_channel_cb_3,
#endif
#if EMBER_CHANNEL_COUNT > 4
    ember_channel_cb_4,
#endif
#if EMBER_CHANNEL_COUNT > 5
    ember_channel_cb_5,
#endif
#if EMBER_CHANNEL_COUNT > 6
    ember_channel_cb_6,
#endif
#if EMBER_CHANNEL_COUNT > 7
    ember_channel_cb_7,
#endif
#if EMBER_CHANNEL_COUNT > 8
    ember_channel_cb_8,
#endif
#if EMBER_CHANNEL_COUNT > 9
    ember_channel_cb_9,
#endif
#if EMBER_CHANNEL_COUNT > 10
    ember_channel_cb_10,
#endif
#if EMBER_CHANNEL_COUNT > 11
    ember_channel_cb_11,
#endif
#if EMBER_CHANNEL_COUNT > 12
    ember_channel_cb_12,
#endif
#if EMBER_CHANNEL_COUNT > 13
    ember_channel_cb_13,
#endif
#if EMBER_CHANNEL_COUNT > 14
    ember_channel_cb_14,
#endif
#if EMBER_CHANNEL_COUNT > 15
    ember_channel_cb_15,
#endif
#if EMBER_CHANNEL_COUNT > 16
    ember_channel_cb_16,
#endif
#if EMBER_CHANNEL_COUNT > 17
    ember_channel_cb_17,
#endif
#if EMBER_CHANNEL_COUNT > 18
    ember_channel_cb_18,
#endif
#if EMBER_CHANNEL_COUNT > 19
    ember_channel_cb_19,
#endif
#if EMBER_CHANNEL_COUNT > 20
    ember_channel_cb_20,
#endif
#if EMBER_CHANNEL_COUNT > 21
    ember_channel_cb_21,
#endif
#if EMBER_CHANNEL_COUNT > 22
    ember_channel_cb_22,
#endif
#if EMBER_CHANNEL_COUNT > 23
    ember_channel_cb_23,
#endif
#if EMBER_CHANNEL_COUNT > 24
    ember_channel_cb_24,
#endif
#if EMBER_CHANNEL_COUNT > 25
    ember_channel_cb_25,
#endif
#if EMBER_CHANNEL_COUNT > 26
    ember_channel_cb_26,
#endif
#if EMBER_CHANNEL_COUNT > 27
    ember_channel_cb_27,
#endif
#if EMBER_CHANNEL_COUNT > 28
    ember_channel_cb_28,
#endif
#if EMBER_CHANNEL_COUNT > 29
    ember_channel_cb_29,
#endif
#if EMBER_CHANNEL_COUNT > 30
    ember_channel_cb_30,
#endif
#if EMBER_CHANNEL_COUNT > 31
    ember_channel_cb_31,
#endif
#if EMBER_CHANNEL_COUNT > 32
    ember_channel_cb_32,
#endif
#if EMBER_CHANNEL_COUNT > 33
    ember_channel_cb_33,
#endif
#if EMBER_CHANNEL_COUNT > 34
    ember_channel_cb_34,
#endif
#if EMBER_CHANNEL_COUNT > 35
    ember_channel_cb_35,
#endif
#if EMBER_CHANNEL_COUNT > 36
    ember_channel_cb_36,
#endif
#if EMBER_CHANNEL_COUNT > 37
    ember_channel_cb_37,
#endif
#if EMBER_CHANNEL_COUNT > 38
    ember_channel_cb_38,
#endif
#if EMBER_CHANNEL_COUNT > 39
    ember_channel_cb_39,
#endif
#if EMBER_CHANNEL_COUNT > 40
    ember_channel_cb_40,
#endif
#if EMBER_CHANNEL_COUNT > 41
    ember_channel_cb_41,
#endif
#if EMBER_CHANNEL_COUNT > 42
    ember_channel_cb_42,
#endif
#if EMBER_CHANNEL_COUNT > 43
    ember_channel_cb_43,
#endif
#if EMBER_CHANNEL_COUNT > 44
    ember_channel_cb_44,
#endif
#if EMBER_CHANNEL_COUNT > 45
    ember_channel_cb_45,
#endif
#if EMBER_CHANNEL_COUNT > 46
    ember_channel_cb_46,
#endif
#if EMBER_CHANNEL_COUNT > 47
    ember_channel_cb_47,
#endif
#if EMBER_CHANNEL_COUNT > 48
    ember_channel_cb_48,
#endif
#if EMBER_CHANNEL_COUNT > 49
    ember_channel_cb_49,
#endif
#if EMBER_CHANNEL_COUNT > 50
    ember_channel_cb_50,
#endif
#if EMBER_CHANNEL_COUNT > 51
    ember_channel_cb_51,
#endif
#if EMBER_CHANNEL_COUNT > 52
    ember_channel_cb_52,
#endif
#if EMBER_CHANNEL_COUNT > 53
    ember_channel_cb_53,
#endif
#if EMBER_CHANNEL_COUNT > 54
    ember_channel_cb_54,
#endif
#if EMBER_CHANNEL_COUNT > 55
    ember_channel_cb_55,
#endif
#if EMBER_CHANNEL_COUNT > 56
    ember_channel_cb_56,
#endif
#if EMBER_CHANNEL_COUNT > 57
    ember_channel_cb_57,
#endif
#if EMBER_CHANNEL_COUNT > 58
    ember_channel_cb_58,
#endif
#if EMBER_CHANNEL_COUNT > 59
    ember_channel_cb_59,
#endif
#if EMBER_CHANNEL_COUNT > 60
    ember_channel_cb_60,
#endif
#if EMBER_CHANNEL_COUNT > 61
    ember_channel_cb_61,
#endif
#if EMBER_CHANNEL_COUNT > 62
    ember_channel_cb_62,
#endif
#if EMBER_CHANNEL_COUNT > 63
    ember_channel_cb_63,
#endif
#if EMBER_CHANNEL_COUNT > 64
    ember_channel_cb_64,
#endif
#if EMBER_CHANNEL_COUNT > 65
    ember_channel_cb_65,
#endif
#if EMBER_CHANNEL_COUNT > 66
    ember_channel_cb_66,
#endif
#if EMBER_CHANNEL_COUNT > 67
    ember_channel_cb_67,
#endif
#if EMBER_CHANNEL_COUNT > 68
    ember_channel_cb_68,
#endif
#if EMBER_CHANNEL_COUNT > 69
    ember_channel_cb_69,
#endif
#if EMBER_CHANNEL_COUNT > 70
    ember_channel_cb_70,
#endif
#if EMBER_CHANNEL_COUNT > 71
    ember_channel_cb_71,
#endif
#if EMBER_CHANNEL_COUNT > 72
    ember_channel_cb_72,
#endif
#if EMBER_CHANNEL_COUNT > 73
    ember_channel_cb_73,
#endif
#if EMBER_CHANNEL_COUNT > 74
    ember_channel_cb_74,
#endif
#if EMBER_CHANNEL_COUNT > 75
    ember_channel_cb_75,
#endif
#if EMBER_CHANNEL_COUNT > 76
    ember_channel_cb_76,
#endif
#if EMBER_CHANNEL_COUNT > 77
    ember_channel_cb_77,
#endif
#if EMBER_CHANNEL_COUNT > 78
    ember_channel_cb_78,
#endif
#if EMBER_CHANNEL_COUNT > 79
    ember_channel_cb_79,
#endif
#if EMBER_CHANNEL_COUNT > 80
    ember_channel_cb_80,
#endif
#if EMBER_CHANNEL_COUNT > 81
    ember_channel_cb_81,
#endif
#if EMBER_CHANNEL_COUNT > 82
    ember_channel_cb_82,
#endif
#if EMBER_CHANNEL_COUNT > 83
    ember_channel_cb_83,
#endif
#if EMBER_CHANNEL_COUNT > 84
    ember_channel_cb_84,
#endif
#if EMBER_CHANNEL_COUNT > 85
    ember_channel_cb_85,
#endif
#if EMBER_CHANNEL_COUNT > 86
    ember_channel_cb_86,
#endif
#if EMBER_CHANNEL_COUNT > 87
    ember_channel_cb_87,
#endif
#if EMBER_CHANNEL_COUNT > 88
    ember_channel_cb_88,
#endif
#if EMBER_CHANNEL_COUNT > 89
    ember_channel_cb_89,
#endif
#if EMBER_CHANNEL_COUNT > 90
    ember_channel_cb_90,
#endif
#if EMBER_CHANNEL_COUNT > 91
    ember_channel_cb_91,
#endif
#if EMBER_CHANNEL_COUNT > 92
    ember_channel_cb_92,
#endif
#if EMBER_CHANNEL_COUNT > 93
    ember_channel_cb_93,
#endif
#if EMBER_CHANNEL_COUNT > 94
    ember_channel_cb_94,
#endif
#if EMBER_CHANNEL_COUNT > 95
    ember_channel_cb_95,
#endif
#if EMBER_CHANNEL_COUNT > 96
    ember_channel_cb_96,
#endif
#if EMBER_CHANNEL_COUNT > 97
    ember_channel_cb_97,
#endif
#if EMBER_CHANNEL_COUNT > 98
    ember_channel_cb_98,
#endif
#if EMBER_CHANNEL_COUNT > 99
    ember_channel_cb_99,
#endif
};

#define EMBER_CHANNEL_CB(channel) \
    void ember_channel_cb_##channel (const EmberIotProp &prop)

#endif
