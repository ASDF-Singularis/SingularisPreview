#pragma once

#include "Modules/ModuleManager.h"

class FSingularisPreviewModule : public IModuleInterface
{
public:
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;
};
