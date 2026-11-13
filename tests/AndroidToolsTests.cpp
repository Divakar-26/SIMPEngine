#include "TestFramework.h"

#include "Android/AndroidTools.h"

#include <filesystem>

TEST_CASE("adb devices output is parsed")
{
    const std::string text =
        "* daemon not running; starting now at tcp:5037\n"
        "* daemon started successfully\n"
        "List of devices attached\n"
        "RG5XYXW8GQ6T4XTO       device usb:1-2 product:spes model:Redmi_Note_11 device:spes transport_id:3\n"
        "emulator-5554          device product:sdk_gphone64_x86_64 model:sdk_gphone64_x86_64 device:emu64xa transport_id:1\n"
        "0123456789ABCDEF       unauthorized usb:1-3 transport_id:5\n"
        "\n";
    std::vector<AndroidTools::Device> devices = AndroidTools::ParseDevices(text);
    CHECK(devices.size() == 3);
    CHECK(devices[0].Serial == "RG5XYXW8GQ6T4XTO");
    CHECK(devices[0].Ready());
    CHECK(!devices[0].Emulator);
    CHECK(devices[0].Model == "Redmi_Note_11");
    CHECK(devices[0].Label() == "Redmi Note 11");
    CHECK(devices[1].Emulator);
    CHECK(devices[1].Label().find("(emulator)") != std::string::npos);
    CHECK(!devices[2].Ready());
    CHECK(devices[2].State == "unauthorized");
}

TEST_CASE("Android package names and ABI lists")
{
    CHECK(AndroidTools::SanitizePackage("My Game!") == "org.simp.mygame");
    CHECK(AndroidTools::SanitizePackage("2048") == "org.simp.game2048");
    CHECK(AndroidTools::SanitizePackage("") == "org.simp.game");
    CHECK(AndroidTools::JoinAbis({"arm64-v8a", "x86_64", "arm64-v8a", ""}) == "arm64-v8a,x86_64");

    std::vector<std::string> avds;
    AndroidTools::ParseAvds("INFO    | Storing crashdata\nPixel_8\nTablet_API_34\n", avds);
    CHECK(avds.size() == 2);
    CHECK(avds[0] == "Pixel_8");
}

TEST_CASE("A folder with platform-tools counts as an Android SDK")
{
    namespace fs = std::filesystem;
    const fs::path root = fs::temp_directory_path() / "simp_fake_sdk";
    std::error_code ec;
    fs::remove_all(root, ec);
    CHECK(!AndroidTools::IsSdkDirectory(root));
    fs::create_directories(root / "platform-tools");
    CHECK(AndroidTools::IsSdkDirectory(root));
    AndroidTools::Toolchain tools = AndroidTools::Detect(root.string());
    CHECK(tools.Sdk == root);
    CHECK(!AndroidTools::SdkCandidates().empty());
    fs::remove_all(root, ec);
}
