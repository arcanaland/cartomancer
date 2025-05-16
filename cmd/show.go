package cmd

import (
	"crypto/md5"
	"fmt"
	"image"
	"image/color" // This is the standard library color package
	_ "image/gif"
	_ "image/jpeg"
	_ "image/png"
	"os"
	"path/filepath"
	"strings"

	"github.com/lucasb-eyer/go-colorful"
	"github.com/nfnt/resize"
	"golang.org/x/term"

	"github.com/arcanaland/cartomancer/internal/card"
	"github.com/arcanaland/cartomancer/internal/config"
	"github.com/arcanaland/cartomancer/internal/deck"

	colorize "github.com/fatih/color" // Rename this import to avoid the conflict
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

	// First try to find existing ANSI art
	var ansiPath string
	ansiFound := false

	// Check for ansi32 directory first
	ansiDir := filepath.Join(deckPath, "ansi32")
	if _, err := os.Stat(ansiDir); !os.IsNotExist(err) {
		if path, err := buildCardPath(ansiDir, parts, ".ansi"); err == nil {
			if _, err := os.Stat(path); !os.IsNotExist(err) {
				ansiPath = path
				ansiFound = true
			}
		}
	}

	// Check for ansi256 directory if ansi32 wasn't found
	if !ansiFound {
		ansiDir = filepath.Join(deckPath, "ansi256")
		if _, err := os.Stat(ansiDir); !os.IsNotExist(err) {
			if path, err := buildCardPath(ansiDir, parts, ".ansi"); err == nil {
				if _, err := os.Stat(path); !os.IsNotExist(err) {
					ansiPath = path
					ansiFound = true
				}
			}
		}
	}

	// If ANSI art was found, return it
	if ansiFound {
		return ansiPath, nil
	}

	// No ANSI art found, look for image files to convert
	imagePath, err := findCardImage(deckPath, parts)
	if err != nil {
		return "", fmt.Errorf("no ANSI art or convertible images found for card: %s", cardID)
	}

	// Generate ANSI art from the image
	cacheDir := filepath.Join(config.GetCacheDir(), "ansi_cache")
	if err := os.MkdirAll(cacheDir, 0755); err != nil {
		return "", fmt.Errorf("failed to create ANSI cache directory: %v", err)
	}

	// Create a cache filename based on the image path
	cacheFilename := fmt.Sprintf("%x.ansi", md5.Sum([]byte(imagePath)))
	cachePath := filepath.Join(cacheDir, cacheFilename)

	// Check if we already have a cached version
	if _, err := os.Stat(cachePath); !os.IsNotExist(err) {
		return cachePath, nil
	}

	// Generate new ANSI art
	if err := generateAnsiArt(imagePath, cachePath); err != nil {
		return "", fmt.Errorf("failed to generate ANSI art: %v", err)
	}

	return cachePath, nil
}

// buildCardPath constructs the path to a card file
func buildCardPath(baseDir string, parts []string, extension string) (string, error) {
	if parts[0] == "major_arcana" && len(parts) == 2 {
		// Major arcana card
		return filepath.Join(baseDir, "major_arcana", parts[1]+extension), nil
	} else if parts[0] == "minor_arcana" && len(parts) == 3 {
		// Minor arcana card
		return filepath.Join(baseDir, "minor_arcana", parts[1], parts[2]+extension), nil
	} else if parts[0] == "custom_cards" && len(parts) >= 3 {
		// Handle custom cards
		if len(parts) == 3 { // Like custom_cards.major_arcana.happy_squirrel
			return filepath.Join(baseDir, parts[0], parts[1], parts[2]+extension), nil
		} else if len(parts) == 4 { // Like custom_cards.minor_arcana.stars.ace
			return filepath.Join(baseDir, parts[0], parts[1], parts[2], parts[3]+extension), nil
		}
	}
	return "", fmt.Errorf("invalid card ID format: %s", strings.Join(parts, "."))
}

// findCardImage searches for an image file for the given card in various directories
func findCardImage(deckPath string, parts []string) (string, error) {
	// Priority order: scalable (SVG), h2400, h1200, h750, any other directories with images
	imageDirs := []string{
		"scalable",
		"h2400",
		"h1200",
		"h750",
	}

	extensions := []string{".svg", ".png", ".jpg", ".jpeg", ".webp"}

	// Try the known directories first
	for _, dir := range imageDirs {
		dirPath := filepath.Join(deckPath, dir)
		if _, err := os.Stat(dirPath); os.IsNotExist(err) {
			continue
		}

		// Try all extensions
		for _, ext := range extensions {
			if path, err := buildCardPath(dirPath, parts, ext); err == nil {
				if _, err := os.Stat(path); !os.IsNotExist(err) {
					return path, nil
				}
			}
		}
	}

	// If not found in standard dirs, try to find any directory containing images
	entries, err := os.ReadDir(deckPath)
	if err != nil {
		return "", err
	}

	for _, entry := range entries {
		if !entry.IsDir() {
			continue
		}

		// Skip already checked directories
		dirName := entry.Name()
		if dirName == "ansi32" || dirName == "ansi256" || dirName == "card_backs" ||
			dirName == "names" || contains(imageDirs, dirName) {
			continue
		}

		dirPath := filepath.Join(deckPath, dirName)
		// Try all extensions
		for _, ext := range extensions {
			if path, err := buildCardPath(dirPath, parts, ext); err == nil {
				if _, err := os.Stat(path); !os.IsNotExist(err) {
					return path, nil
				}
			}
		}
	}

	return "", fmt.Errorf("no image found for card")
}

// contains checks if a string is in a slice
func contains(slice []string, item string) bool {
	for _, s := range slice {
		if s == item {
			return true
		}
	}
	return false
}

// generateAnsiArt converts an image file to ANSI art and saves it to the specified output path
func generateAnsiArt(imagePath, outputPath string) error {
	// Open the image file
	file, err := os.Open(imagePath)
	if err != nil {
		return fmt.Errorf("failed to open image: %v", err)
	}
	defer file.Close()

	// Decode the image
	img, _, err := image.Decode(file)
	if err != nil {
		return fmt.Errorf("failed to decode image: %v", err)
	}

	// Generate ANSI art
	ansiArt, err := imageToAnsi(img, 40, 32, true)
	if err != nil {
		return fmt.Errorf("failed to convert image to ANSI: %v", err)
	}

	// Write to file
	if err := os.WriteFile(outputPath, []byte(ansiArt), 0644); err != nil {
		return fmt.Errorf("failed to write ANSI art to file: %v", err)
	}

	return nil
}

// imageToAnsi converts an image to ANSI art
func imageToAnsi(img image.Image, width, height int, use256Colors bool) (string, error) {
	// Resize image to desired dimensions (doubled for half-block characters)
	resized := resize.Resize(uint(width*2), uint(height*2), img, resize.Lanczos3)

	// Create a buffer for the ANSI output
	var buffer strings.Builder

	// Process the image
	for y := 0; y < height*2; y += 2 {
		for x := 0; x < width*2; x += 2 {
			// Get the four pixels that will make up one character cell
			c1 := getColorAt(resized, x, y)
			c2 := getColorAt(resized, x+1, y)
			c3 := getColorAt(resized, x, y+1)
			c4 := getColorAt(resized, x+1, y+1)

			// Use the upper half block character for simplicity and reliability
			// Top pixels as foreground, bottom pixels as background
			col1, _ := colorful.MakeColor(c1)
			col2, _ := colorful.MakeColor(c2)
			col3, _ := colorful.MakeColor(c3)
			col4, _ := colorful.MakeColor(c4)

			// Calculate average colors
			upperHalfFg := averageColor(col1, col2)
			lowerHalfBg := averageColor(col3, col4)

			// Convert to standard colors
			fg := colorfulToColor(upperHalfFg)
			bg := colorfulToColor(lowerHalfBg)

			// Append to buffer with the upper half block character
			buffer.WriteString(ansiColorString('▀', fg, bg, use256Colors))
		}
		buffer.WriteString("\n")
	}

	return buffer.String(), nil
}

// getColorAt returns the color at a specific coordinate
func getColorAt(img image.Image, x, y int) color.Color {
	bounds := img.Bounds()
	if x >= bounds.Min.X && x < bounds.Max.X && y >= bounds.Min.Y && y < bounds.Max.Y {
		return img.At(x, y)
	}
	return color.RGBA{0, 0, 0, 255} // Return black for out-of-bounds
}

// averageColor calculates the average of multiple colors
func averageColor(colors ...colorful.Color) colorful.Color {
	var r, g, b float64
	for _, c := range colors {
		r += c.R
		g += c.G
		b += c.B
	}
	count := float64(len(colors))
	return colorful.Color{R: r / count, G: g / count, B: b / count}
}

// colorfulToColor converts a colorful.Color to a standard color.Color
func colorfulToColor(c colorful.Color) color.Color {
	// Always return direct RGB values rather than mapping
	r := uint8(c.R * 255)
	g := uint8(c.G * 255)
	b := uint8(c.B * 255)

	return color.RGBA{R: r, G: g, B: b, A: 255}
}

// ansiColorString formats a character with ANSI color codes
func ansiColorString(char rune, fg, bg color.Color, use256Colors bool) string {
	// Get RGB values for foreground and background
	r1, g1, b1, _ := fg.RGBA()
	r2, g2, b2, _ := bg.RGBA()

	// Convert from uint32 to uint8 (RGBA() returns values in range 0-65535)
	r1, g1, b1 = r1>>8, g1>>8, b1>>8
	r2, g2, b2 = r2>>8, g2>>8, b2>>8

	if use256Colors {
		return fmt.Sprintf("\x1b[38;2;%d;%d;%dm\x1b[48;2;%d;%d;%dm%c\x1b[0m",
			r1, g1, b1, r2, g2, b2, char)
	}

	// Simplified 16-color version as fallback
	return string(char)
}

// loadAnsiArt loads the ANSI art from a file
func loadAnsiArt(path string) (string, error) {
	data, err := os.ReadFile(path)
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

	infoLines = append(infoLines, colorize.CyanString("Card: ")+colorize.HiWhiteString("%s", c.Name))

	infoLines = append(infoLines, colorize.CyanString("Deck: ")+colorize.HiWhiteString(deckName))
	infoLines = append(infoLines, colorize.CyanString("ID:   ")+colorize.HiWhiteString(c.ID))

	if c.Type == "major_arcana" {
		infoLines = append(infoLines, colorize.CyanString("Type: ")+
			colorize.HiWhiteString("Major Arcana · %s", arcanaSymbol))
	} else {
		infoLines = append(infoLines, colorize.CyanString("Type: ")+
			colorize.HiWhiteString("Minor Arcana · %s", arcanaSymbol))
		infoLines = append(infoLines, colorize.CyanString("Suit: ")+
			colorize.HiWhiteString("%s · %s", c.Suit, suitSymbol))
		infoLines = append(infoLines, colorize.CyanString("Rank: ")+colorize.HiWhiteString(c.Rank))
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
		infoLines = append(infoLines, colorize.CyanString("Description:"))
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
