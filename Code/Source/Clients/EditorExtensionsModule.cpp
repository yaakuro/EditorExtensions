
#include <EditorExtensions/EditorExtensionsTypeIds.h>
#include <EditorExtensionsModuleInterface.h>
#include "EditorExtensionsSystemComponent.h"

namespace EditorExtensions
{
    class EditorExtensionsModule
        : public EditorExtensionsModuleInterface
    {
    public:
        AZ_RTTI(EditorExtensionsModule, EditorExtensionsModuleTypeId, EditorExtensionsModuleInterface);
        AZ_CLASS_ALLOCATOR(EditorExtensionsModule, AZ::SystemAllocator);
    };
}// namespace EditorExtensions

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME), EditorExtensions::EditorExtensionsModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_EditorExtensions, EditorExtensions::EditorExtensionsModule)
#endif
