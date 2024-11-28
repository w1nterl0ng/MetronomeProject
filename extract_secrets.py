Import("env")
import configparser
import os

# Read secrets.ini file
config = configparser.ConfigParser()
secrets_path = os.path.join(env.subst("$PROJECT_DIR"), "secrets.ini")

if os.path.exists(secrets_path):
    config.read(secrets_path)

    # Create build flags from secrets
    build_flags = [
        f'-DWIFI_SSID=\\"{config["secrets"]["WIFI_SSID"]}\\"',
        f'-DWIFI_PASSWORD=\\"{config["secrets"]["WIFI_PASSWORD"]}\\"',
    ]

    # Get existing build flags
    existing_flags = env.get("BUILD_FLAGS", [])

    # Combine existing flags with our new ones
    env.Replace(BUILD_FLAGS=existing_flags + build_flags)
else:
    print("Warning: secrets.ini not found. Using default values.")
    env.Append(
        BUILD_FLAGS=[
            '-DWIFI_SSID=\\"default_ssid\\"',
            '-DWIFI_PASSWORD=\\"default_password\\"',
        ]
    )
