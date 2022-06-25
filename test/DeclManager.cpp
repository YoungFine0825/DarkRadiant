#include "RadiantTest.h"

#include "ideclmanager.h"
#include "testutil/TemporaryFile.h"

namespace test
{

using DeclManagerTest = RadiantTest;

class TestDeclaration :
    public decl::IDeclaration
{
private:
    decl::Type _type;
    std::string _name;
    decl::DeclarationBlockSyntax _block;

public:
    TestDeclaration(decl::Type type, const std::string& name) :
        _type(type),
        _name(name)
    {}

    const std::string& getDeclName() const override
    {
        return _name;
    }

    decl::Type getDeclType() const override
    {
        return _type;
    }

    const decl::DeclarationBlockSyntax& getBlockSyntax() const override
    {
        return _block;
    }

    void setBlockSyntax(const decl::DeclarationBlockSyntax& block) override
    {
        _block = block;
    }
};

class TestDeclarationCreator :
    public decl::IDeclarationCreator
{
public:
    bool processingDisabled = false;

    decl::Type getDeclType() const override
    {
        return decl::Type::Material;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        while (processingDisabled)
        {
            using namespace std::chrono_literals;
            std::this_thread::sleep_for(20ms);
        }

        return std::make_shared<TestDeclaration>(getDeclType(), name);
    }
};

class TestDeclaration2Creator :
    public decl::IDeclarationCreator
{
public:
    decl::Type getDeclType() const override
    {
        return decl::Type::Model;
    }

    decl::IDeclaration::Ptr createDeclaration(const std::string& name) override
    {
        return std::make_shared<TestDeclaration>(getDeclType(), name);
    }
};

TEST_F(DeclManagerTest, DeclTypeRegistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    EXPECT_NO_THROW(GlobalDeclarationManager().registerDeclType("testdecl", creator));

    // Registering the same type name twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("testdecl", creator), std::logic_error);

    // Passing a new creator instance doesn't help either
    auto creator2 = std::make_shared<TestDeclarationCreator>();
    EXPECT_THROW(GlobalDeclarationManager().registerDeclType("testdecl", creator2), std::logic_error);
}

TEST_F(DeclManagerTest, DeclTypeUnregistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    // Unregistering the creator should succeed
    EXPECT_NO_THROW(GlobalDeclarationManager().unregisterDeclType("testdecl"));

    // Trying to unregister it twice should result in an exception
    EXPECT_THROW(GlobalDeclarationManager().unregisterDeclType("testdecl"), std::logic_error);
}

inline std::set<std::string> getAllDeclNames(decl::Type type)
{
    // Iterate over all decls and collect the names
    std::set<std::string> foundNames;

    GlobalDeclarationManager().foreachDeclaration(type, [&](const decl::IDeclaration& declaration)
    {
        foundNames.insert(declaration.getDeclName());
    });

    return foundNames;
}

inline void checkKnownTestDeclNames()
{
    auto foundNames = getAllDeclNames(decl::Type::Material);

    // Check a few known ones
    EXPECT_TRUE(foundNames.count("decl/exporttest/guisurf1") > 0);
    EXPECT_TRUE(foundNames.count("decl/numbers/0") > 0);
    EXPECT_TRUE(foundNames.count("decl/numbers/1") > 0);
    EXPECT_TRUE(foundNames.count("decl/numbers/2") > 0);

    // decltables should not be listed
    EXPECT_FALSE(foundNames.count("decltable1") > 0);
    EXPECT_FALSE(foundNames.count("decltable2") > 0);
    EXPECT_FALSE(foundNames.count("decltable3") > 0);
}

inline void checkKnownTestDecl2Names()
{
    auto foundNames = getAllDeclNames(decl::Type::Model);

    // Assume testdecls are not listed
    EXPECT_FALSE(foundNames.count("decl/exporttest/guisurf1") > 0);
    EXPECT_FALSE(foundNames.count("decl/numbers/0") > 0);
    EXPECT_FALSE(foundNames.count("decl/numbers/1") > 0);
    EXPECT_FALSE(foundNames.count("decl/numbers/2") > 0);

    // testdecl2 should be listed
    EXPECT_TRUE(foundNames.count("decltable1") > 0);
    EXPECT_TRUE(foundNames.count("decltable2") > 0);
    EXPECT_TRUE(foundNames.count("decltable3") > 0);
}

TEST_F(DeclManagerTest, DeclFolderRegistration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls/", "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithoutSlash)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Omit the trailing slash, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", "decl");

    checkKnownTestDeclNames();
}

TEST_F(DeclManagerTest, DeclFolderRegistrationWithExtensionDot)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Add the dot to the file extension, should work just fine
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    checkKnownTestDeclNames();
}

// Test a second decl creator
TEST_F(DeclManagerTest, DeclTypeCreatorRegistration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    checkKnownTestDeclNames();
    checkKnownTestDecl2Names();
}

// Test that a creator coming late to the party is immediately fed with the buffered decl blocks
TEST_F(DeclManagerTest, LateCreatorRegistration)
{
    auto creator = std::make_shared<TestDeclarationCreator>();

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    // Let the testdecl creator finish its work
    getAllDeclNames(decl::Type::Material);

    auto foundTestDecl2Names = getAllDeclNames(decl::Type::Model);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable2") > 0);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable3") > 0);

    // Register the testdecl2 creator now, it should be used by the decl manager to parse the missing pieces
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // Everything should be registered now
    checkKnownTestDecl2Names();
}

TEST_F(DeclManagerTest, CreatorRegistrationDuringRunningThread)
{
    auto creator = std::make_shared<TestDeclarationCreator>();

    // Hold back this creator until we let it go in this fixture
    creator->processingDisabled = true;

    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    auto foundTestDecl2Names = getAllDeclNames(decl::Type::Model);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);

    // Register the testdecl2 creator now, it should be used by the decl manager to parse the missing pieces
    GlobalDeclarationManager().registerDeclType("testdecl2", std::make_shared<TestDeclaration2Creator>());

    // The first thread is still running, so we didn't get any unrecognised decls reported
    foundTestDecl2Names = getAllDeclNames(decl::Type::Model);
    EXPECT_FALSE(foundTestDecl2Names.count("decltable1") > 0);

    // Let the testdecl creator finish its work
    creator->processingDisabled = false;
    getAllDeclNames(decl::Type::Material);

    // Everything should be registered now
    checkKnownTestDecl2Names();
}

TEST_F(DeclManagerTest, DeclsReloadedSignal)
{
    auto creator = std::make_shared<TestDeclarationCreator>();
    GlobalDeclarationManager().registerDeclType("testdecl", creator);

    bool materialSignalFired = false;
    bool modelSignalFired = false;
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Material).connect(
        [&]() { materialSignalFired = true; }
    );
    GlobalDeclarationManager().signal_DeclsReloaded(decl::Type::Model).connect(
        [&]() { modelSignalFired = true; }
    );

    // Parse this folder, it contains decls of type testdecl and testdecl2 in the .decl files
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    // Force the thread to be finished
    GlobalDeclarationManager().foreachDeclaration(decl::Type::Material, [](const decl::IDeclaration&) {});

    EXPECT_TRUE(materialSignalFired) << "Material signal should have fired by the time parsing has finished";
    EXPECT_FALSE(modelSignalFired) << "Model-type Signal should not have been fired";
}

TEST_F(DeclManagerTest, FindDeclaration)
{
    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/exporttest/guisurf1"));
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/nonexistent"));
}

TEST_F(DeclManagerTest, ReloadDeclarationWithChangedFile)
{
    TemporaryFile tempFile(_context.getTestProjectPath() + "testdecls/temp_file.decl");
    tempFile.setContents(R"(

decl/temporary/11
{
    diffusemap textures/temporary/11
}

decl/temporary/12
{
    diffusemap textures/temporary/12
}

)");

    GlobalDeclarationManager().registerDeclType("testdecl", std::make_shared<TestDeclarationCreator>());
    GlobalDeclarationManager().registerDeclFolder(decl::Type::Material, "testdecls", ".decl");

    auto temp12 = GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/temporary/12");
    EXPECT_TRUE(temp12) << "Couldn't find the declaration decl/temporary/12";

    auto temp11 = GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/temporary/11");
    EXPECT_TRUE(temp11) << "Couldn't find the declaration decl/temporary/11";

    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/temporary/13"))
        << "decl/temporary/13 should not be present";

    EXPECT_NE(temp12->getBlockSyntax().contents.find("diffusemap textures/temporary/12"), std::string::npos) <<
        "Didn't find the expected contents in the decl block";

    // Change the file, change temp12, remove temp11 and add temp13 instead
    tempFile.setContents(R"(

decl/temporary/12
{
    diffusemap textures/changed_temporary/12
}

decl/temporary/13
{
    diffusemap textures/temporary/13
}

)");

    // Check the change sin temp12
    temp12 = GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/temporary/12");
    EXPECT_TRUE(temp12) << "Couldn't find the declaration decl/temporary/12";

    EXPECT_NE(temp12->getBlockSyntax().contents.find("diffusemap textures/changed_temporary/12"), std::string::npos) <<
        "Couldn't find the changed contents in the decl block";

    EXPECT_TRUE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/temporary/13"))
        << "decl/temporary/13 should be present now";
    EXPECT_FALSE(GlobalDeclarationManager().findDeclaration(decl::Type::Material, "decl/temporary/11"))
        << "decl/temporary/11 should be gone now";
}

}
