#pragma once

// "Leaky" Form; ensures result won't get clobbered
const char* LeakStr(const char* fmt, ...);

// Use this to explictly say "i know that this string lives in a circular buffer
// and I'm OK with that".
const char* TmpStr(const char* fmt, ...);
