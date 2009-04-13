#pragma once
#include "Command.h"

/**
 * \ingroup TortoiseProc
 * Renames files and folders.
 */
class SendMailCommand : public Command
{
public:
	/**
	 * Executes the command.
	 */
	virtual bool			Execute();
};
