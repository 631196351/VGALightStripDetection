#include "utility.h"


std::string RunCmd(std::string strCmd)
{

	FILE *fp = NULL;
	char data[128] = {0};
	std::string r;

	//strCmd += " 2>&1";

	//SPDLOG_SINKS_DEBUG("cmd : {}", strCmd);

	fp = popen(strCmd.c_str(), "r");
	if (NULL == fp)
	{
		return "-1";
	}
	while (fgets(data, sizeof(data), fp) != NULL)
	{
		r += data;
	}
	pclose(fp);

	// SPDLOG_SINKS_DEBUG("cmd result: {}", r);
	return r;
}

/*定时系统在(1-59)秒之后 reboot or shutdown*/
void shutdownAfter(unsigned int secons, bool reboot)
{
	time_t t;
    struct tm * lt;
	char cmd[32]={0};
    time (&t);          //获取Unix时间戳。
 
	if(secons > 59)return;
    
    t -= (60-secons);   //先将系统时间向前调整
    lt = localtime (&t);//转为时间结构。
	sprintf (cmd,"date -s %02d:%02d:%02d",lt->tm_hour, lt->tm_min, lt->tm_sec);
	RunCmd(cmd);

	if(reboot)
		RunCmd("shutdown -r +1");  //定时1分钟之后关机
	else
		RunCmd("shutdown +1");  //定时1分钟之后关机
	
	t += (60-secons);              //调整回系统时间
	lt = localtime (&t);         
	sprintf (cmd,"date -s %02d:%02d:%02d",lt->tm_hour, lt->tm_min, lt->tm_sec);
	RunCmd(cmd);
}