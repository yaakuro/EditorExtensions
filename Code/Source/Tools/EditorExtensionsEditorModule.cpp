
#include <EditorExtensions/EditorExtensionsTypeIds.h>
#include <EditorExtensionsModuleInterface.h>
#include "EditorExtensionsEditorSystemComponent.h"

void InitEditorExtensionsResources()
{
    // We must register our Qt resources (.qrc file) since this is being loaded from a separate module (gem)
    Q_INIT_RESOURCE(EditorExtensions);
    Q_INIT_RESOURCE(CppGenerator);
    Q_INIT_RESOURCE(AttachmentPreview);
    Q_INIT_RESOURCE(ComponentPreset);
}

namespace EditorExtensions
{
    class EditorExtensionsEditorModule
        : public EditorExtensionsModuleInterface
    {
    public:
        AZ_RTTI(EditorExtensionsEditorModule, EditorExtensionsEditorModuleTypeId, EditorExtensionsModuleInterface);
        AZ_CLASS_ALLOCATOR(EditorExtensionsEditorModule, AZ::SystemAllocator);

        EditorExtensionsEditorModule()
        {
            InitEditorExtensionsResources();

            // Push results of [MyComponent]::CreateDescriptor() into m_descriptors here.
            // Add ALL components descriptors associated with this gem to m_descriptors.
            // This will associate the AzTypeInfo information for the components with the the SerializeContext, BehaviorContext and EditContext.
            // This happens through the [MyComponent]::Reflect() function.
            m_descriptors.insert(m_descriptors.end(), {
                EditorExtensionsEditorSystemComponent::CreateDescriptor(),
            });
        }

        /**
         * Add required SystemComponents to the SystemEntity.
         * Non-SystemComponents should not be added here
         */
        AZ::ComponentTypeList GetRequiredSystemComponents() const override
        {
            return AZ::ComponentTypeList {
                azrtti_typeid<EditorExtensionsEditorSystemComponent>(),
            };
        }
    };
}// namespace EditorExtensions

#if defined(O3DE_GEM_NAME)
AZ_DECLARE_MODULE_CLASS(AZ_JOIN(Gem_, O3DE_GEM_NAME, _Editor), EditorExtensions::EditorExtensionsEditorModule)
#else
AZ_DECLARE_MODULE_CLASS(Gem_EditorExtensions_Editor, EditorExtensions::EditorExtensionsEditorModule)
#endif
