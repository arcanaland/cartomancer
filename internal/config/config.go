package config

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/BurntSushi/toml"
)

// Config represents the application configuration
type Config struct {
	DefaultDeck string `toml:"default_deck"`
}

// GetXDGDataHome returns XDG_DATA_HOME or default path
func GetXDGDataHome() string {
	if xdgData := os.Getenv("XDG_DATA_HOME"); xdgData != "" {
		return xdgData
	}
	homeDir, err := os.UserHomeDir()
	if err != nil {
		return ""
	}
	return filepath.Join(homeDir, ".local", "share")
}

// GetXDGConfigHome returns XDG_CONFIG_HOME or default path
func GetXDGConfigHome() string {
	if xdgConfig := os.Getenv("XDG_CONFIG_HOME"); xdgConfig != "" {
		return xdgConfig
	}
	homeDir, err := os.UserHomeDir()
	if err != nil {
		return ""
	}
	return filepath.Join(homeDir, ".config")
}

// GetDeckLibraryPath returns the path to the deck library
func GetDeckLibraryPath() string {
	return filepath.Join(GetXDGDataHome(), "tarot", "decks")
}

// GetConfigFilePath returns the path to the config file
func GetConfigFilePath() string {
	return filepath.Join(GetXDGConfigHome(), "cartomancer", "config.toml")
}

// LoadConfig loads the config file
func LoadConfig() (*Config, error) {
	configPath := GetConfigFilePath()

	// Create default config if it doesn't exist
	if _, err := os.Stat(configPath); os.IsNotExist(err) {
		return createDefaultConfig()
	}

	var config Config
	_, err := toml.DecodeFile(configPath, &config)
	if err != nil {
		return nil, fmt.Errorf("error decoding config file: %v", err)
	}

	return &config, nil
}

// createDefaultConfig creates a default config file
func createDefaultConfig() (*Config, error) {
	configPath := GetConfigFilePath()
	configDir := filepath.Dir(configPath)

	// Ensure the config directory exists
	if err := os.MkdirAll(configDir, 0755); err != nil {
		return nil, fmt.Errorf("error creating config directory: %v", err)
	}

	// Create default config
	config := &Config{
		DefaultDeck: "rider-waite-smith", // Default deck
	}

	// Create the file
	file, err := os.Create(configPath)
	if err != nil {
		return nil, fmt.Errorf("error creating config file: %v", err)
	}
	defer file.Close()

	// Encode the config to TOML
	encoder := toml.NewEncoder(file)
	if err := encoder.Encode(config); err != nil {
		return nil, fmt.Errorf("error encoding config: %v", err)
	}

	return config, nil
}

// GetDeckPath returns the path to a deck, either in the deck library or a relative path
func GetDeckPath(deckName string) (string, error) {
	// First, try to find the deck in the deck library
	libraryPath := GetDeckLibraryPath()
	deckPath := filepath.Join(libraryPath, deckName)

	if _, err := os.Stat(deckPath); err == nil {
		return deckPath, nil
	}

	// If not found in the library, treat as a relative path
	if _, err := os.Stat(deckName); err == nil {
		return deckName, nil
	}

	return "", fmt.Errorf("deck not found: %s", deckName)
}

// GetDefaultDeck returns the default deck name from config
func GetDefaultDeck() (string, error) {
	config, err := LoadConfig()
	if err != nil {
		return "", err
	}

	return config.DefaultDeck, nil
}

// SetDefaultDeck sets the default deck in the config
func SetDefaultDeck(deckName string) error {
	config, err := LoadConfig()
	if err != nil {
		return err
	}

	// Update the default deck
	config.DefaultDeck = deckName

	// Open the config file for writing
	configPath := GetConfigFilePath()
	file, err := os.Create(configPath)
	if err != nil {
		return fmt.Errorf("error opening config file: %v", err)
	}
	defer file.Close()

	// Encode the updated config
	encoder := toml.NewEncoder(file)
	if err := encoder.Encode(config); err != nil {
		return fmt.Errorf("error encoding config: %v", err)
	}

	return nil
}
