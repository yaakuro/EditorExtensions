
#pragma once

namespace EditorExtensions
{
    // System Component TypeIds
    inline constexpr const char* EditorExtensionsSystemComponentTypeId = "{30F881AD-9F00-4A47-B1CE-22161EFB9776}";
    inline constexpr const char* EditorExtensionsEditorSystemComponentTypeId = "{31863360-0025-4CAC-99D3-FBB95051EB8A}";

    // Module derived classes TypeIds
    inline constexpr const char* EditorExtensionsModuleInterfaceTypeId = "{650DFB7C-C0E7-4F13-AC8F-F3EC08F56226}";
    inline constexpr const char* EditorExtensionsModuleTypeId = "{C96EFAB0-1F59-449E-8378-43092E9ECD38}";
    // The Editor Module by default is mutually exclusive with the Client Module
    // so they use the Same TypeId
    inline constexpr const char* EditorExtensionsEditorModuleTypeId = EditorExtensionsModuleTypeId;

    // Interface TypeIds
    inline constexpr const char* EditorExtensionsRequestsTypeId = "{47A672CC-551A-45C8-BA7F-36D86ACD473F}";
} // namespace EditorExtensions
