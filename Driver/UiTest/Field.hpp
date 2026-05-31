#pragma once

#include "Core/Utils.hpp"

namespace DeviceField {
	namespace UiTest {
		inline const auto Id = lstr("__UI_Test__");
		inline const auto Name = lstr("Ui 测试设备");
		extern
		#if defined(RINGAPP_DRIVER_UI_TEST_EXPORT)
			Q_DECL_EXPORT	
		#else
			Q_DECL_IMPORT
		#endif
		const QString ModuleName;

	// property for uitest bytes.
	#define UiTest_Bytes uitest_bytes
	// property for uitest enabled.
	#define UiTest_Enabled uitest_enabled
		inline const auto Enabled = AS_STR(UiTest_Enabled);
		inline const auto Bytes = AS_STR(UiTest_Bytes);
		
		inline const int 
			RequestSample = 0x01,
			SignalECG = 0x01,
			SignalPPG = 0x02,
			SignalEEG = 0x03,
			SignalCap = 0x04,
			SignalNoseNoise = 0x05;
	}
		
}

