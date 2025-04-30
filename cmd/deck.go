package cmd

import (
	"fmt"
	"os"
	"path/filepath"

	"github.com/arcanaland/cartomancer/internal/config"
	"github.com/arcanaland/cartomancer/internal/deck"
	"github.com/spf13/cobra"
)

// deckCmd represents the deck command group
var deckCmd = &cobra.Command{
	Use:   "deck",
	Short: "Manage tarot decks in your deck library",
	Long:  `Commands for managing tarot decks in your deck library.`,
}

// deckListCmd represents the deck list command
var deckListCmd = &cobra.Command{
	Use:   "ls",
	Short: "List available decks in your deck library",
	Run: func(cmd *cobra.Command, args []string) {
		libraryPath, err := filepath.EvalSymlinks(config.GetDeckLibraryPath())
		if err != nil {
			fmt.Printf("Error resolving symbolic link: %v\n", err)
			return
		}

		// Check if deck library exists
		if _, err := os.Stat(libraryPath); os.IsNotExist(err) {
			fmt.Printf("Deck library at %s does not exist.\n", libraryPath)
			fmt.Println("Run 'cartomancer deck init' to create it.")
			return
		}

		// Get default deck
		defaultDeck, err := config.GetDefaultDeck()
		if err != nil {
			fmt.Printf("Error getting default deck: %v\n", err)
			return
		}

		// Read the deck library directory
		entries, err := os.ReadDir(libraryPath)
		if err != nil {
			fmt.Printf("Error reading deck library: %v\n", err)
			return
		}

		if len(entries) == 0 {
			fmt.Println("No decks found in your deck library.")
			fmt.Println("You can add decks by copying them to:", libraryPath)
			return
		}

		for _, entry := range entries {
			// Resolve the symbolic link or regular entry
			entryPath := filepath.Join(libraryPath, entry.Name())
			fileInfo, err := os.Stat(entryPath)
			if err != nil {
				fmt.Printf("Error resolving entry %s: %v\n", entry.Name(), err)
				continue
			}

			// Check if the resolved entry is a directory
			if fileInfo.IsDir() {
				deckPath := entryPath
				d, err := deck.LoadDeck(deckPath)

				if err != nil {
					// Not a valid deck, skip
					continue
				}

				if entry.Name() == defaultDeck {
					fmt.Printf("* %s (%s) [DEFAULT]\n", entry.Name(), d.Name)
				} else {
					fmt.Printf("  %s (%s)\n", entry.Name(), d.Name)
				}
			}
		}
	},
}

// deckSetDefaultCmd represents the deck set-default command
var deckSetDefaultCmd = &cobra.Command{
	Use:   "set-default [deck_name]",
	Short: "Set the default deck",
	Args:  cobra.ExactArgs(1),
	Run: func(cmd *cobra.Command, args []string) {
		deckName := args[0]

		// Check if the deck exists
		deckPath, err := config.GetDeckPath(deckName)
		if err != nil {
			fmt.Printf("Error: %v\n", err)
			return
		}

		// Try to load the deck to make sure it's valid
		_, err = deck.LoadDeck(deckPath)
		if err != nil {
			fmt.Printf("Error: Not a valid deck - %v\n", err)
			return
		}

		// Set as default
		err = config.SetDefaultDeck(deckName)
		if err != nil {
			fmt.Printf("Error setting default deck: %v\n", err)
			return
		}

		fmt.Printf("Default deck set to: %s\n", deckName)
	},
}

// deckInitCmd represents the deck init command
var deckInitCmd = &cobra.Command{
	Use:   "init",
	Short: "Initialize the deck library",
	Run: func(cmd *cobra.Command, args []string) {
		libraryPath := config.GetDeckLibraryPath()

		// Create the deck library directory if it doesn't exist
		if err := os.MkdirAll(libraryPath, 0755); err != nil {
			fmt.Printf("Error creating deck library: %v\n", err)
			return
		}

		fmt.Println("Deck library initialized at:", libraryPath)
		fmt.Println("You can now add decks by copying them to this directory.")

		// Initialize config
		_, err := config.LoadConfig()
		if err != nil {
			fmt.Printf("Error initializing config: %v\n", err)
			return
		}

		configPath := config.GetConfigFilePath()
		fmt.Println("Config file initialized at:", configPath)
	},
}

func init() {
	RootCmd.AddCommand(deckCmd)
	deckCmd.AddCommand(deckListCmd)
	deckCmd.AddCommand(deckSetDefaultCmd)
	deckCmd.AddCommand(deckInitCmd)
}
