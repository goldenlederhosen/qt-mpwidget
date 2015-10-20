#include "qprocess_meta.h"

char const *ProcessError_2_latin1str(QProcess::ProcessError e)
{
    switch(e) {
        case QProcess::FailedToStart:
            return "The process failed to start. Either the invoked program is missing, or you may have insufficient permissions to invoke the program.";

        case QProcess::Crashed:
            return "The process crashed some time after starting successfully.";

        case QProcess::Timedout:
            return "The last waitFor...() function timed out. The state of QProcess is unchanged, and you can try calling waitFor...() again.";

        case QProcess::WriteError:
            return "An error occurred when attempting to write to the process. For example, the process may not be running, or it may have closed its input channel.";

        case QProcess::ReadError:
            return "An error occurred when attempting to read from the process. For example, the process may not be running.";

        case QProcess::UnknownError:
            return "An unknown error occurred. This is the default return value of error().";

        default:
            return "unknown error";
    }
}
