/**
*@file CProcessManager.h
*@brief This file defined processes manager.
*@author antmuse@live.cn
*@date 2014-10-22
*/

#ifndef APP_CPROCESSMANAGER_H
#define APP_CPROCESSMANAGER_H

//#include "HConfig.h"
#include "CProcessHandle.h"
#include "irrArray.h"
#include "irrMap.h"
#include "path.h"


namespace irr {
class CPipe;

/**
*@class CProcessManager
*@brief This class provides methods for working with processes.
* All methods is static.
*
* Usage example:
*@code
*     CPipe outPipe;
*     CProcessManager::DProcessParam args;
*     CProcessHandle* ph = CProcessManager::launch("/bin/ps", args, 0, &outPipe, 0);
*     if(ph){
*           ... // read from pipe.
*           s32 rc = ph.wait();
*     }
*@endcode
*/
class CProcessManager {
public:
    typedef core::array<io::path> DProcessParam;
    typedef core::map<io::path, io::path> DProcessEnvronment;


    /**
    *@brief Get ID of current process.
    *@return The ID of the current process.
    */
    static CProcessHandle::PID getCurrentID();

    /**
    *@brief Get times spent by current process.
    *@param userTime Time in user mode, in seconds.
    *@param kernelTime Time in kernel mode, in seconds.
    *@return The number of seconds spent by the current process in user and kernel mode.
    */
    static void times(long& userTime, long& kernelTime);

    /**
    *@brief Create a new process for the given command.
    *@param command Command.
    *@param args The given arguments are passed to the command on the command line.
    *@return The handle of process if success, else 0.
    */
    static CProcessHandle* launch(const io::path& command, const DProcessParam& args);


    /**
    *@brief Create a new process for the given command.
    *@param command Command.
    *@param args The given arguments are passed to the command on the command line.
    *@param initialDirectory The directory that process starts executing in.
    *@return The handle of process if success, else 0.
    */
    static CProcessHandle* launch(const io::path& command, const DProcessParam& args, const io::path& initialDirectory);


    /**
    *@brief Create a new process for the given command.
    *@note: The same pipe can be used for both out pipe and error pipe.
    * After a pipe has been passed as input pipe, only write operations are valid.
    * After a pipe has been passed as output pipe or error pipe, only read operations are valid.
    * It is forbidden to pass the same pipe as input pipe and output pipe or error pipe.
    *@param command Command.
    *@param args The given arguments are passed to the command on the command line.
    *@param inPipe If non-null, the corresponding standard input stream
    *of the launched process is redirected to this pipe.
    *@param outPipe If non-null, the corresponding standard output stream
    *of the launched process is redirected to this pipe.
    *@param errPipe If non-null, the corresponding standard error stream
    *of the launched process is redirected to this pipe.
    *@return The handle of process if success, else 0.
    */
    static CProcessHandle* launch(
        const io::path& command,
        const DProcessParam& args,
        CPipe* inPipe,
        CPipe* outPipe,
        CPipe* errPipe);



    /**
    *@brief Create a new process for the given command.
    *@note: The same pipe can be used for both out pipe and error pipe.
    * After a pipe has been passed as input pipe, only write operations are valid.
    * After a pipe has been passed as output pipe or error pipe, only read operations are valid.
    * It is forbidden to pass the same pipe as input pipe and output pipe or error pipe.
    *@param command Command.
    *@param args The given arguments are passed to the command on the command line.
    *@param initialDirectory The directory that process starts executing in.
    *@param inPipe If non-null, the corresponding standard input stream
    *of the launched process is redirected to this pipe.
    *@param outPipe If non-null, the corresponding standard output stream
    *of the launched process is redirected to this pipe.
    *@param errPipe If non-null, the corresponding standard error stream
    *of the launched process is redirected to this pipe.
    *@return The handle of process if success, else 0.
    */
    static CProcessHandle* launch(
        const io::path& command,
        const DProcessParam& args,
        const io::path& initialDirectory,
        CPipe* inPipe,
        CPipe* outPipe,
        CPipe* errPipe);



    /**
    *@brief Create a new process for the given command.
    *@note: The same pipe can be used for both out pipe and error pipe.
    * After a pipe has been passed as input pipe, only write operations are valid.
    * After a pipe has been passed as output pipe or error pipe, only read operations are valid.
    * It is forbidden to pass the same pipe as input pipe and output pipe or error pipe.
    *@param command Command.
    *@param args The given arguments are passed to the command on the command line.
    *@param inPipe If non-null, the corresponding standard input stream
    *of the launched process is redirected to this pipe.
    *@param outPipe If non-null, the corresponding standard output stream
    *of the launched process is redirected to this pipe.
    *@param errPipe If non-null, the corresponding standard error stream
    *of the launched process is redirected to this pipe.
    *@param env The specified environment variables to launch process.
    *@return The handle of process if success, else 0.
    */
    static CProcessHandle* launch(
        const io::path& command,
        const DProcessParam& args,
        CPipe* inPipe,
        CPipe* outPipe,
        CPipe* errPipe,
        const DProcessEnvronment& env);


    /**
    *@brief Create a new process for the given command.
    *@note: The same pipe can be used for both out pipe and error pipe.
    * After a pipe has been passed as input pipe, only write operations are valid.
    * After a pipe has been passed as output pipe or error pipe, only read operations are valid.
    * It is forbidden to pass the same pipe as input pipe and output pipe or error pipe.
    *@param command Command.
    *@param args The given arguments are passed to the command on the command line.
    *@param initialDirectory The directory that process starts executing in.
    *@param inPipe If non-null, the corresponding standard input stream
    *of the launched process is redirected to this pipe.
    *@param outPipe If non-null, the corresponding standard output stream
    *of the launched process is redirected to this pipe.
    *@param errPipe If non-null, the corresponding standard error stream
    *of the launched process is redirected to this pipe.
    *@param env The specified environment variables to launch process.
    *@return The handle of process if success, else 0.
    */
    static CProcessHandle* launch(
        const io::path& command,
        const DProcessParam& args,
        const io::path& initialDirectory,
        CPipe* inPipe,
        CPipe* outPipe,
        CPipe* errPipe,
        const DProcessEnvronment& env);


    /**
    *@brief Waits for the process specified by handle to terminate.
    *@return The exit code of the process.
    */
    static s32 wait(const CProcessHandle& handle);


    /**
    *@brief check if the process specified by handle is running or not
    * This is preferable on Windows where process ID may be reused.
    *@param handle The process handle.
    *@return true if running, else false.
    */
    static bool isRunning(const CProcessHandle& handle);


    /**
    *@brief Check if the process specified by given pid is running or not.
    * This is preferable on Windows where process ID may be reused.
    *@param pid The process ID.
    *@return true if running, else false.
    */
    static bool isRunning(CProcessHandle::PID pid);


    /**
    *@brief Kill the process specified by handle.
    * This is preferable on Windows where process ID may be reused.
    *@param pid The process handle.
    */
    static void kill(CProcessHandle& handle);

    /**
    *@brief Kill the process with the given ID.
    *@param pid The process ID.
    */
    static void kill(CProcessHandle::PID pid);


private:
    static core::array<fschar_t> getEnvironmentVariablesBuffer(const DProcessEnvronment& env);

};

} //end namespace irr


#endif //APP_CPROCESSMANAGER_H
