#include "Iroha.h"

int main()
{
	YAML::Node config = YAML::LoadFile("Config.yaml");

	auto key = config["Key"].as<std::string>();
	auto token = config["Token"].as<std::string>();

	fmt::print("Key: {} - Token {}\n", key, token);

	return 0;
}
