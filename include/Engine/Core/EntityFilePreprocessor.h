#include <xercesc/framework/psvi/XSElementDeclaration.hpp>
#include <xercesc/framework/psvi/XSComplexTypeDefinition.hpp>
#include <xercesc/framework/psvi/XSParticle.hpp>
#include <xercesc/framework/psvi/XSModelGroup.hpp>
#include <xercesc/framework/psvi/XSAnnotation.hpp>
#include <xercesc/framework/psvi/XSValue.hpp>
#include <xercesc/framework/MemBufFormatTarget.hpp>
#include <xercesc/framework/MemBufInputSource.hpp>
#include <xercesc/parsers/XercesDOMParser.hpp>

#include "Util/XercesString.h"
#include "ResourceManager.h"
#include "World.h"
#include "EntityFile.h"

class EntityFilePreprocessor
{
public:
    EntityFilePreprocessor(const EntityFile* entityFile);

    void RegisterComponents(World* world);

private:
    const EntityFile* m_EntityFile;
    std::map<std::string, unsigned int> m_ComponentCounts;
	std::map<std::string, ComponentInfo> m_ComponentInfo;

	void onStartComponent(EntityID entity, std::string type);
    void parseComponentInfo();
    void parseDefaults();
};