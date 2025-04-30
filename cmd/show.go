package cmd

import (
	"fmt"
	"io/ioutil"
	"os"
	"path/filepath"
	"strings"

	"golang.org/x/term"

	"github.com/arcanaland/cartomancer/internal/card"
	"github.com/arcanaland/cartomancer/internal/config"

	"github.com/arcanaland/cartomancer/internal/deck"
	"github.com/fatih/color"
	"github.com/spf13/cobra"
)

var showCmd = &cobra.Command{
	Use:   "show [card_id]",
	Short: "Display information about a specific card with ANSI art",
	Long: `Show displays detailed information about a tarot card with ANSI terminal art.
Use canonical card IDs like 'major_arcana.00' or 'minor_arcana.wands.ace'.

You can specify a deck using the --deck flag, which will look for the deck
in your deck library (XDG_DATA_HOME/tarot/decks) or as a relative path.
If no deck is specified, the default deck from your config will be used.

Examples:
  cartomancer show major_arcana.00
  cartomancer show --deck rider-waite-smith minor_arcana.wands.ace
  cartomancer show --deck ./custom-deck major_arcana.01`,
	Args: cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		cardID := args[0]

		// Get deck flag value
		deckFlag, _ := cmd.Flags().GetString("deck")

		var deckPath string
		var err error

		if deckFlag != "" {
			// User specified a deck
			deckPath, err = config.GetDeckPath(deckFlag)
			if err != nil {
				return err
			}
		} else {
			// Use default deck from config
			defaultDeck, err := config.GetDefaultDeck()
			if err != nil {
				return fmt.Errorf("error getting default deck: %v", err)
			}

			deckPath, err = config.GetDeckPath(defaultDeck)
			if err != nil {
				return fmt.Errorf("error loading default deck: %v", err)
			}
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

	showCmd.Flags().StringP("deck", "d", "", "Specify a deck from your deck library or a path to a deck")
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

func getSuitSymbol(suit string) string {
	switch suit {
	case "wands":
		return ""
	case "cups":
		return ""
	case "swords":
		return "󰞇"
	case "pentacles":
		return "󱙧"
	default:
		return "•"
	}
}

// getArcanaSymbol returns a symbol for the arcana type
func getArcanaSymbol(isMinor bool) string {
	if isMinor {
		return "󱀝"
	}
	return ""
}

// wrapText wraps text to a specified width
func wrapText(text string, width int) []string {
	// Ensure width is reasonable
	if width < 10 {
		width = 40 // Use a sensible default if width is too small
	}

	var result []string
	var currentLine string
	words := strings.Fields(text)

	if len(words) == 0 {
		return []string{""}
	}

	for _, word := range words {
		// Check if adding this word would exceed the width
		if len(currentLine) == 0 {
			// First word on the line, always add it
			currentLine = word
		} else if len(currentLine)+1+len(word) <= width {
			// Word fits on current line with a space
			currentLine += " " + word
		} else {
			// Word doesn't fit, start a new line
			result = append(result, currentLine)
			currentLine = word
		}
	}

	// Add the last line if not empty
	if currentLine != "" {
		result = append(result, currentLine)
	}

	return result
}

// displayCard displays the card information with ANSI art
func displayCard(c *card.Card, ansiArt, deckName string) {
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

	// Get terminal width
	width, _, err := term.GetSize(int(os.Stdout.Fd()))
	if err != nil || width <= 0 {
		width = 80 // Default if we can't get terminal width
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
	infoLines = append(infoLines, color.CyanString("ID:   ")+color.HiWhiteString(c.ID))

	if c.Type == "major_arcana" {
		infoLines = append(infoLines, color.CyanString("Type: ")+
			color.HiWhiteString("Major Arcana · %s", arcanaSymbol))
	} else {
		infoLines = append(infoLines, color.CyanString("Type: ")+
			color.HiWhiteString("Minor Arcana · %s", arcanaSymbol))
		infoLines = append(infoLines, color.CyanString("Suit: ")+
			color.HiWhiteString("%s · %s", c.Suit, suitSymbol))
		infoLines = append(infoLines, color.CyanString("Rank: ")+color.HiWhiteString(c.Rank))
	}

	// Calculate layout
	// We'll display the ANSI art on the left and info on the right
	spacing := 4
	infoStartCol := maxAnsiWidth + spacing

	// Calculate available width for text, ensuring it's at least 20 characters
	infoWidth := width - infoStartCol - 2 // Leave a small margin
	if infoWidth < 20 {
		infoWidth = 20 // Minimum width for text
	}

	// Add description with word wrapping
	if c.AltText != "" {
		infoLines = append(infoLines, "")
		infoLines = append(infoLines, color.CyanString("Description:"))
		// Wrap the description text to fit in the available width
		descLines := wrapText(c.AltText, infoWidth)
		infoLines = append(infoLines, descLines...)
	}

	// Print the header
	fmt.Println()

	// Print each line
	maxLines := max(len(ansiLines), len(infoLines))
	for i := 0; i < maxLines; i++ {
		// Print 2-character wide left padding
		fmt.Print("  ")
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
