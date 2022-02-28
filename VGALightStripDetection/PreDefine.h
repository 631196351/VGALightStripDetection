#pragma once
// 全局的预定义变量定义文件
// 主要 LEDColor 定义
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
// 已廢棄
enum
{
	W_Threshold = 250,
	R_Threshold = 250,
	G_Threshold = 250,
	B_Threshold = 250
};

// 定义一个存放结果的文件夹name
#define AgingFolder "aging_rect_image"

// 可以在log里看到程式为何退出
enum
{
	eNotExit = 0,	//未侦测到退出指令
	eExit = 1,		//执行完毕，正常退出指令
	eExitWithKey = 2,	//侦测到退出指令
	eExitWithException = 3	// 异常退出
};

// 关机指令
enum
{
	ePowerOff = 0,	// 程式执行完延时关机
	eNotPowerOff = -1,	// 程式执行完不关机
	eReStart = -2	// 程式执行完重启
};

// version
enum
{
	VersionMajor = 3,
	VersionSec = 0,
	VersionThi = 2,
	VersionMin = 5
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

// 最大支持多少摄像头
enum 
{
	CaptureNum = 3
};

// 定义摄像头下标(未用)
enum
{
	eCamera_Overhead,	//上
	eCamera_Front,		//前
	eCamera_Rear,		//后
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