#pragma once

#ifdef HY_DEBUG
	#define HY_ENABLE_ASSERTS
#endif

#define HY_ENABLE_VERIFY

#ifdef HY_ENABLE_ASSERTS
	#define HY_ASSERT_NO_MESSAGE(condition) { if(!(condition)) { HY_ERROR("Assertion Failed"); __debugbreak(); } }
	#define HY_ASSERT_MESSAGE(condition, ...) { if(!(condition)) { HY_ERROR("Assertion Failed: {0}", __VA_ARGS__); __debugbreak(); } }

	#define HY_ASSERT_RESOLVE(arg1, arg2, macro, ...) macro
	#define HY_GET_ASSERT_MACRO(...) HY_EXPAND_VARGS(HY_ASSERT_RESOLVE(__VA_ARGS__, HY_ASSERT_MESSAGE, HY_ASSERT_NO_MESSAGE))

	#define HY_ASSERT(...) HY_EXPAND_VARGS( HY_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
	#define HY_CORE_ASSERT(...) HY_EXPAND_VARGS( HY_GET_ASSERT_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
	#define HY_ASSERT(...)
	#define HY_CORE_ASSERT(...)
#endif

#ifdef HY_ENABLE_VERIFY
#define HY_VERIFY_NO_MESSAGE(condition) { if(!(condition)) { HY_ERROR("Verify Failed"); __debugbreak(); } }
#define HY_VERIFY_MESSAGE(condition, ...) { if(!(condition)) { HY_ERROR("Verify Failed: {0}", __VA_ARGS__); __debugbreak(); } }

#define HY_VERIFY_RESOLVE(arg1, arg2, macro, ...) macro
#define HY_GET_VERIFY_MACRO(...) HY_EXPAND_VARGS(HY_VERIFY_RESOLVE(__VA_ARGS__, HY_VERIFY_MESSAGE, HY_VERIFY_NO_MESSAGE))

#define HY_VERIFY(...) HY_EXPAND_VARGS( HY_GET_VERIFY_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#define HY_CORE_VERIFY(...) HY_EXPAND_VARGS( HY_GET_VERIFY_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
#define HY_VERIFY(...)
#define HY_CORE_VERIFY(...)
#endif