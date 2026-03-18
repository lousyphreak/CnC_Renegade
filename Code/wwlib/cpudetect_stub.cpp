#include "cpudetect.h"

StringClass CPUDetectClass::ProcessorLog;
StringClass CPUDetectClass::CompactLog;

int CPUDetectClass::ProcessorType = 0;
int CPUDetectClass::ProcessorFamily = 0;
int CPUDetectClass::ProcessorModel = 0;
int CPUDetectClass::ProcessorRevision = 0;
int CPUDetectClass::ProcessorSpeed = 0;
__int64 CPUDetectClass::ProcessorTicksPerSecond = 1000;
double CPUDetectClass::InvProcessorTicksPerSecond = 1.0 / 1000.0;

unsigned CPUDetectClass::FeatureBits = 0;
unsigned CPUDetectClass::ExtendedFeatureBits = 0;

unsigned CPUDetectClass::L2CacheSize = 0;
unsigned CPUDetectClass::L2CacheLineSize = 0;
unsigned CPUDetectClass::L2CacheSetAssociative = 0;
unsigned CPUDetectClass::L1DataCacheSize = 0;
unsigned CPUDetectClass::L1DataCacheLineSize = 0;
unsigned CPUDetectClass::L1DataCacheSetAssociative = 0;
unsigned CPUDetectClass::L1InstructionCacheSize = 0;
unsigned CPUDetectClass::L1InstructionCacheLineSize = 0;
unsigned CPUDetectClass::L1InstructionCacheSetAssociative = 0;
unsigned CPUDetectClass::L1InstructionTraceCacheSize = 0;
unsigned CPUDetectClass::L1InstructionTraceCacheSetAssociative = 0;

unsigned CPUDetectClass::TotalPhysicalMemory = 0;
unsigned CPUDetectClass::AvailablePhysicalMemory = 0;
unsigned CPUDetectClass::TotalPageMemory = 0;
unsigned CPUDetectClass::AvailablePageMemory = 0;
unsigned CPUDetectClass::TotalVirtualMemory = 0;
unsigned CPUDetectClass::AvailableVirtualMemory = 0;

unsigned CPUDetectClass::OSVersionNumberMajor = 0;
unsigned CPUDetectClass::OSVersionNumberMinor = 0;
unsigned CPUDetectClass::OSVersionBuildNumber = 0;
unsigned CPUDetectClass::OSVersionPlatformId = 0;
StringClass CPUDetectClass::OSVersionExtraInfo;

bool CPUDetectClass::HasCPUIDInstruction = false;
bool CPUDetectClass::HasRDTSCInstruction = false;
bool CPUDetectClass::HasSSESupport = false;
bool CPUDetectClass::HasSSE2Support = false;
bool CPUDetectClass::HasCMOVSupport = false;
bool CPUDetectClass::HasMMXSupport = false;
bool CPUDetectClass::Has3DNowSupport = false;
bool CPUDetectClass::HasExtended3DNowSupport = false;

CPUDetectClass::ProcessorManufacturerType CPUDetectClass::ProcessorManufacturer = CPUDetectClass::MANUFACTURER_UNKNOWN;
CPUDetectClass::IntelProcessorType CPUDetectClass::IntelProcessor = CPUDetectClass::INTEL_PROCESSOR_UNKNOWN;
CPUDetectClass::AMDProcessorType CPUDetectClass::AMDProcessor = CPUDetectClass::AMD_PROCESSOR_UNKNOWN;
CPUDetectClass::VIAProcessorType CPUDetectClass::VIAProcessor = CPUDetectClass::VIA_PROCESSOR_UNKNOWN;
CPUDetectClass::RiseProcessorType CPUDetectClass::RiseProcessor = CPUDetectClass::RISE_PROCESSOR_UNKNOWN;

char CPUDetectClass::VendorID[20] = "UNKNOWN";
char CPUDetectClass::ProcessorString[48] = "Bootstrap CPU detection stub";

const char * CPUDetectClass::Get_Processor_Manufacturer_Name()
{
    return "Unknown";
}

bool CPUDetectClass::CPUID(
    unsigned & u_eax_,
    unsigned & u_ebx_,
    unsigned & u_ecx_,
    unsigned & u_edx_,
    unsigned)
{
    u_eax_ = 0;
    u_ebx_ = 0;
    u_ecx_ = 0;
    u_edx_ = 0;
    return false;
}
