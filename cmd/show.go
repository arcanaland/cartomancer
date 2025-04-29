package cmd

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"github.com/arcanaland/cartomancer/internal/card"
	"github.com/arcanaland/cartomancer/internal/deck"
	"github.com/fatih/color"
	"github.com/spf13/cobra"
)

// showCmd represents the show command
var showCmd = &cobra.Command{
	Use:   "show [card_id] [deck_path]",
	Short: "Display information about a specific card with ANSI art",
	Long: `Show displays detailed information about a tarot card with ANSI terminal art.
Use canonical card IDs like 'major_arcana.00' or 'minor_arcana.wands.ace'.
If deck_path is not specified, it will use the current directory.

Example:
  cartomancer show minor_arcana.wands.ace ./rider-waite-smith`,
	Args: cobra.RangeArgs(1, 2),
	RunE: func(cmd *cobra.Command, args []string) error {
		cardID := args[0]
		deckPath := "."
		if len(args) > 1 {
			deckPath = args[1]
		}

		// Check if path exists
		if _, err := os.Stat(deckPath); os.IsNotExist(err) {
			return fmt.Errorf("deck directory not found: %s", deckPath)
		}

		// Load the deck
		d, err := deck.LoadDeck(deckPath)
		if err != nil {
			return fmt.Errorf("error loading deck: %v", err)
		}

		// Get the card
		c, err := d.GetCard(cardID)
		if err != nil {
			return fmt.Errorf("error getting card: %v", err)
		}

		// Get the ANSI art
		ansiPath, err := findAnsiFile(deckPath, cardID)
		if err != nil {
			return fmt.Errorf("error finding ANSI art: %v", err)
		}

		ansiArt, err := loadAnsiArt(ansiPath)
		if err != nil {
			return fmt.Errorf("error loading ANSI art: %v", err)
		}

		// Display the card info with ANSI art
		displayCard(c, ansiArt, d.Name)

		return nil
	},
}

func init() {
	RootCmd.AddCommand(showCmd)
}

// findAnsiFile finds the path to the ANSI art file for a card
func findAnsiFile(deckPath, cardID string) (string, error) {
	// Parse the card ID
	parts := strings.Split(cardID, ".")
	if len(parts) < 2 {
		return "", fmt.Errorf("invalid card ID format: %s", cardID)
	}

	// Check for ansi32 directory first
	ansiDir := filepath.Join(deckPath, "ansi32")
	if _, err := os.Stat(ansiDir); os.IsNotExist(err) {
		// Check for ansi256 directory
		ansiDir = filepath.Join(deckPath, "ansi256")
		if _, err := os.Stat(ansiDir); os.IsNotExist(err) {
			return "", fmt.Errorf("no ANSI art directories found")
		}
	}

	var ansiPath string
	if parts[0] == "major_arcana" && len(parts) == 2 {
		// Major arcana card
		ansiPath = filepath.Join(ansiDir, "major_arcana", parts[1]+".ansi")
	} else if parts[0] == "minor_arcana" && len(parts) == 3 {
		// Minor arcana card
		ansiPath = filepath.Join(ansiDir, "minor_arcana", parts[1], parts[2]+".ansi")
	} else {
		return "", fmt.Errorf("invalid card ID format: %s", cardID)
	}

	if _, err := os.Stat(ansiPath); os.IsNotExist(err) {
		return "", fmt.Errorf("ANSI art file not found: %s", ansiPath)
	}

	return ansiPath, nil
}

// loadAnsiArt loads the ANSI art from a file
func loadAnsiArt(path string) (string, error) {
	data, err := ioutil.ReadFile(path)
	if err != nil {
		return "", err
	}
	return string(data), nil
}

// getSuitSymbol returns a symbol for a suit
func getSuitSymbol(suit string) string {
	switch suit {
	case "wands":
		return "🔥" // Fire symbol
	case "cups":
		return "💧" // Water symbol
	case "swords":
		return "💨" // Air symbol
	case "pentacles":
		return "🌍" // Earth symbol
	default:
		return "•"
	}
}

// getArcanaSymbol returns a symbol for the arcana type
func getArcanaSymbol(isMinor bool) string {
	if isMinor {
		return "♢" // Diamond for minor arcana
	}
	return "★" // Star for major arcana
}

// displayCard displays the card information with ANSI art
func displayCard(c *card.Card, ansiArt, deckName string) {
	// Clear the screen
	fmt.Print("\033[H\033[2J")

	// Split the ANSI art into lines
	ansiLines := strings.Split(ansiArt, "\n")
	maxAnsiWidth := 0
	for _, line := range ansiLines {
		// Calculate the visible width (excluding ANSI escape sequences)
		visibleWidth := len(stripAnsi(line))
		if visibleWidth > maxAnsiWidth {
			maxAnsiWidth = visibleWidth
		}
	}

	// Prepare the info lines
	var infoLines []string

	// Get symbols
	var arcanaSymbol, suitSymbol string
	isMinor := c.Type == "minor_arcana"

	arcanaSymbol = getArcanaSymbol(isMinor)
	if isMinor {
		suitSymbol = getSuitSymbol(c.Suit)
	}

	infoLines = append(infoLines, color.CyanString("Card: ")+color.HiWhiteString("%s", c.Name))

	infoLines = append(infoLines, color.CyanString("Deck: ")+color.HiWhiteString(deckName))
	infoLines = append(infoLines, color.CyanString("ID: ")+color.HiWhiteString(c.ID))

	if c.Type == "major_arcana" {
		infoLines = append(infoLines, color.CyanString("Type: ")+
			color.HiWhiteString("%s Major Arcana", arcanaSymbol))
	} else {
		infoLines = append(infoLines, color.CyanString("Type: ")+
			color.HiWhiteString("%s Minor Arcana", arcanaSymbol))
		infoLines = append(infoLines, color.CyanString("Suit: ")+
			color.HiWhiteString("%s %s", suitSymbol, c.Suit))
		infoLines = append(infoLines, color.CyanString("Rank: ")+color.HiWhiteString(c.Rank))
	}

	if c.AltText != "" {
		infoLines = append(infoLines, "")
		infoLines = append(infoLines, color.CyanString("Description:"))
		infoLines = append(infoLines, c.AltText)
	}

	// Calculate layout
	// We'll display the ANSI art on the left and info on the right
	spacing := 4
	infoStartCol := maxAnsiWidth + spacing

	// Print the header
	fmt.Println()

	// Print each line
	maxLines := max(len(ansiLines), len(infoLines))
	for i := 0; i < maxLines; i++ {
		// Print ANSI art line if available
		if i < len(ansiLines) {
			fmt.Print(ansiLines[i])
			// Pad to infoStartCol
			visibleWidth := len(stripAnsi(ansiLines[i]))
			fmt.Print(strings.Repeat(" ", infoStartCol-visibleWidth))
		} else {
			fmt.Print(strings.Repeat(" ", infoStartCol))
		}

		// Print info line if available
		if i < len(infoLines) {
			fmt.Print(infoLines[i])
		}

		fmt.Println()
	}

	fmt.Println()
}

// stripAnsi removes ANSI escape sequences from a string
func stripAnsi(s string) string {
	var result strings.Builder
	inEscape := false
	for _, c := range s {
		if inEscape {
			if c == 'm' {
				inEscape = false
			}
		} else if c == '\033' {
			inEscape = true
		} else {
			result.WriteRune(c)
		}
	}
	return result.String()
}

// max returns the maximum of two integers
func max(a, b int) int {
	if a > b {
		return a
	}
	return b
}
