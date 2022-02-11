#pragma once

enum LEDColor
{
	BLUE,
	GREEN,
	RED,
	WHITE,
	BGR = WHITE,
	AllColor,
	BGRW = AllColor,
	BLACK,
};

static const char* color_str[] = { "Blue", "Green", "Red", "White" };

// 用来过滤像素点的阈值
enum
{
	W_Threshold = 250,
	R_Threshold = 250,
	G_Threshold = 250,
	B_Threshold = 250
};

#define AgingFolder "aging_rect_image"

enum
{
	eNotExit = 0,	//未侦测到退出指令
	eExit = 1,		//执行完毕，正常退出指令
	eExitWithKey = 2,	//侦测到退出指令
	eExitWithException = 3	// 异常退出
};

enum
{
	ePowerOff = 0,	// 程式执行完延时关机
	eNotPowerOff = -1,	// 程式执行完不关机
	eReStart = -2	// 程式执行完重启
};

enum
{
	VersionMajor = 3,
	VersionSec = 0,
	VersionThi = 2,
	VersionMin = 6
};

enum HSV
{
	eHmin = 0,
	eHmax = 1,
	eSmin = 2,
	eVmin = 3,
	eHmin2 = 4,
	eHmax2 = 5
};

enum 
{
	CaptureNum = 3
};

enum
{
	eCamera_Overhead,
	eCamera_Front,
	eCamera_Rear,
	eCamera_All
};


#define EXCEPTION_OPERATOR_TRY try

#define EXCEPTION_OPERATOR_CATCH_ERRORCODE_1	\
catch (ErrorCode& e)						\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
}

#define EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_1	\
catch (cv::Exception& e)					\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
	throw e;								\
}

#define EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_1  \
catch (std::exception& e)					\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
	throw e;								\
}

#define EXCEPTION_OPERATOR_CATCH_ERRORCODE_2	\
catch (ErrorCode& e)						\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
	throw e;								\
}

#define EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_2 \
		EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_1

#define EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_2 \
		EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_1


#define EXCEPTION_OPERATOR_CATCH_ERRORCODE_3	\
catch (ErrorCode& e)						\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
	showErrorCode(e);						\
}

#define EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_3	\
catch (cv::Exception& e)					\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
	showErrorCode(ErrorCode(e, ERR_OPENCV_RUNTIME_EXCEPTION));	\
}

#define EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_3 \
catch (std::exception& e)					\
{											\
	SPDLOG_NOTES_THIS_FUNC_EXCEPTION;		\
	showErrorCode(ErrorCode(e, ERR_STD_EXCEPTION));	\
}


#define EXCEPTION_OPERATOR_CATCH_1				\
EXCEPTION_OPERATOR_CATCH_ERRORCODE_1			\
EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_1			\
EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_1

#define EXCEPTION_OPERATOR_CATCH_2				\
EXCEPTION_OPERATOR_CATCH_ERRORCODE_2			\
EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_2			\
EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_2

#define EXCEPTION_OPERATOR_CATCH_3				\
EXCEPTION_OPERATOR_CATCH_ERRORCODE_3			\
EXCEPTION_OPERATOR_CATCH_CV_EXCEPTION_3			\
EXCEPTION_OPERATOR_CATCH_STD_EXCEPTION_3