package validator

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/BurntSushi/toml"
)

type ValidationResults struct {
	Errors   []string
	Warnings []string
}

type Validator struct {
	DeckPath string
	Results  ValidationResults
}

func NewValidator(deckPath string) *Validator {
	return &Validator{
		DeckPath: deckPath,
		Results:  ValidationResults{},
	}
}

func (v *Validator) Validate() (ValidationResults, error) {
	if err := v.validateDeckToml(); err != nil {
		return v.Results, err
	}

	v.validateDirectoryStructure()
	v.validateCardBacks()
	v.validateMajorArcana()
	v.validateMinorArcana()
	v.validateNames()
	v.validateAnsiArt()

	return v.Results, nil
}

func (v *Validator) validateDeckToml() error {
	deckTomlPath := filepath.Join(v.DeckPath, "deck.toml")
	if _, err := os.Stat(deckTomlPath); os.IsNotExist(err) {
		return fmt.Errorf("deck.toml not found in %s", v.DeckPath)
	}

	var deckConfig DeckConfig
	if _, err := toml.DecodeFile(deckTomlPath, &deckConfig); err != nil {
		return fmt.Errorf("error parsing deck.toml: %v", err)
	}

	if deckConfig.Deck.ID == "" {
		v.Results.Errors = append(v.Results.Errors, "deck.id is required in deck.toml")
	}

	if deckConfig.Deck.Name == "" {
		v.Results.Errors = append(v.Results.Errors, "deck.name is required in deck.toml")
	}

	if deckConfig.Deck.Version == "" {
		v.Results.Errors = append(v.Results.Errors, "deck.version is required in deck.toml")
	}

	if deckConfig.Deck.SchemaVersion == "" {
		v.Results.Errors = append(v.Results.Errors, "deck.schema_version is required in deck.toml")
	} else if deckConfig.Deck.SchemaVersion != "1.0" {
		v.Results.Errors = append(v.Results.Errors,
			fmt.Sprintf("unsupported schema_version: %s (supported: 1.0)", deckConfig.Deck.SchemaVersion))
	}

	// Validate card backs
	if deckConfig.CardBacks != nil {
		if len(deckConfig.CardBacks.Variants) > 1 && deckConfig.CardBacks.Default == "" {
			v.Results.Errors = append(v.Results.Errors,
				"card_backs.default is required when multiple card back variants are defined")
		}

		for variantName, variant := range deckConfig.CardBacks.Variants {
			if variant.Image == "" {
				v.Results.Errors = append(v.Results.Errors,
					fmt.Sprintf("card_backs.variants.%s.image is required", variantName))
			} else {
				imagePath := filepath.Join(v.DeckPath, variant.Image)
				if _, err := os.Stat(imagePath); os.IsNotExist(err) {
					v.Results.Errors = append(v.Results.Errors,
						fmt.Sprintf("card back image not found: %s", variant.Image))
				}
			}
		}
	}
	return nil
}

// validateDirectoryStructure checks if the deck has the expected directory structure
func (v *Validator) validateDirectoryStructure() {
	// Check for card_backs directory
	cardBacksDir := filepath.Join(v.DeckPath, "card_backs")
	if _, err := os.Stat(cardBacksDir); os.IsNotExist(err) {
		v.Results.Warnings = append(v.Results.Warnings, "card_backs directory not found")
	}

	// Check for at least one image directory (h*, scalable)
	foundImageDir := false

	// Check for scalable directory
	scalableDir := filepath.Join(v.DeckPath, "scalable")
	if _, err := os.Stat(scalableDir); err == nil {
		foundImageDir = true
	}

	// Check for raster directories (h*)
	entries, err := os.ReadDir(v.DeckPath)
	if err == nil {
		for _, entry := range entries {
			if entry.IsDir() && strings.HasPrefix(entry.Name(), "h") {
				if _, err := fmt.Sscanf(entry.Name(), "h%d", new(int)); err == nil {
					foundImageDir = true
					break
				}
			}
		}
	}

	if !foundImageDir {
		v.Results.Errors = append(v.Results.Errors,
			"no image directories found (expecting scalable/ or h*/ directories)")
	}

	// Check for names directory
	namesDir := filepath.Join(v.DeckPath, "names")
	if _, err := os.Stat(namesDir); os.IsNotExist(err) {
		v.Results.Warnings = append(v.Results.Warnings, "names directory not found")
	}
}

// validateCardBacks checks if card backs exist and are valid
func (v *Validator) validateCardBacks() {
	cardBacksDir := filepath.Join(v.DeckPath, "card_backs")
	if _, err := os.Stat(cardBacksDir); os.IsNotExist(err) {
		return // Already warned about missing directory
	}

	// Check if at least one card back exists
	entries, err := os.ReadDir(cardBacksDir)
	if err != nil {
		v.Results.Errors = append(v.Results.Errors,
			fmt.Sprintf("error reading card_backs directory: %v", err))
		return
	}

	if len(entries) == 0 {
		v.Results.Errors = append(v.Results.Errors, "no card backs found in card_backs directory")
	}
}

// validateMajorArcana checks if major arcana cards exist
func (v *Validator) validateMajorArcana() {
	// Find the image directories
	imageDirs := []string{}
	scalableDir := filepath.Join(v.DeckPath, "scalable")
	if _, err := os.Stat(scalableDir); err == nil {
		imageDirs = append(imageDirs, scalableDir)
	}

	entries, err := os.ReadDir(v.DeckPath)
	if err == nil {
		for _, entry := range entries {
			if entry.IsDir() && strings.HasPrefix(entry.Name(), "h") {
				if _, err := fmt.Sscanf(entry.Name(), "h%d", new(int)); err == nil {
					imageDirs = append(imageDirs, filepath.Join(v.DeckPath, entry.Name()))
				}
			}
		}
	}

	// Check major arcana in all image directories
	foundMajorArcana := false
	for _, imageDir := range imageDirs {
		majorArcanaDir := filepath.Join(imageDir, "major_arcana")
		if _, err := os.Stat(majorArcanaDir); os.IsNotExist(err) {
			continue
		}

		foundMajorArcana = true
		// Check for all 22 major arcana cards (00-21)
		missingCards := []string{}
		for i := 0; i <= 21; i++ {
			cardName := fmt.Sprintf("%02d", i)
			found := false

			// Check for common image extensions
			for _, ext := range []string{".svg", ".png", ".jpg", ".jpeg", ".webp"} {
				cardPath := filepath.Join(majorArcanaDir, cardName+ext)
				if _, err := os.Stat(cardPath); err == nil {
					found = true
					break
				}
			}

			if !found {
				missingCards = append(missingCards, cardName)
			}
		}

		if len(missingCards) > 0 {
			v.Results.Errors = append(v.Results.Errors,
				fmt.Sprintf("missing major arcana cards in %s: %s", imageDir, strings.Join(missingCards, ", ")))
		}
	}

	if !foundMajorArcana {
		v.Results.Errors = append(v.Results.Errors, "major_arcana directory not found in any image directory")
	}
}

// validateMinorArcana checks if minor arcana cards exist
func (v *Validator) validateMinorArcana() {
	// Find the image directories
	imageDirs := []string{}
	scalableDir := filepath.Join(v.DeckPath, "scalable")
	if _, err := os.Stat(scalableDir); err == nil {
		imageDirs = append(imageDirs, scalableDir)
	}

	entries, err := os.ReadDir(v.DeckPath)
	if err == nil {
		for _, entry := range entries {
			if entry.IsDir() && strings.HasPrefix(entry.Name(), "h") {
				if _, err := fmt.Sscanf(entry.Name(), "h%d", new(int)); err == nil {
					imageDirs = append(imageDirs, filepath.Join(v.DeckPath, entry.Name()))
				}
			}
		}
	}

	// Check minor arcana in all image directories
	foundMinorArcana := false
	for _, imageDir := range imageDirs {
		minorArcanaDir := filepath.Join(imageDir, "minor_arcana")
		if _, err := os.Stat(minorArcanaDir); os.IsNotExist(err) {
			continue
		}

		foundMinorArcana = true

		// Check for all four suits
		suits := []string{"wands", "cups", "swords", "pentacles"}
		cardRanks := []string{
			"ace", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
			"page", "knight", "queen", "king",
		}

		for _, suit := range suits {
			suitDir := filepath.Join(minorArcanaDir, suit)
			if _, err := os.Stat(suitDir); os.IsNotExist(err) {
				v.Results.Errors = append(v.Results.Errors,
					fmt.Sprintf("missing suit directory: %s in %s", suit, minorArcanaDir))
				continue
			}

			// Check for all 14 cards in each suit
			missingCards := []string{}
			for _, rank := range cardRanks {
				found := false

				// Check for common image extensions
				for _, ext := range []string{".svg", ".png", ".jpg", ".jpeg", ".webp"} {
					cardPath := filepath.Join(suitDir, rank+ext)
					if _, err := os.Stat(cardPath); err == nil {
						found = true
						break
					}
				}

				if !found {
					missingCards = append(missingCards, rank)
				}
			}

			if len(missingCards) > 0 {
				v.Results.Errors = append(v.Results.Errors,
					fmt.Sprintf("missing cards in %s suit: %s", suit, strings.Join(missingCards, ", ")))
			}
		}
	}

	if !foundMinorArcana {
		v.Results.Errors = append(v.Results.Errors, "minor_arcana directory not found in any image directory")
	}
}

// validateNames checks localization files
func (v *Validator) validateNames() {
	namesDir := filepath.Join(v.DeckPath, "names")
	if _, err := os.Stat(namesDir); os.IsNotExist(err) {
		return // Already warned about missing directory
	}

	// Check if at least one language file exists
	entries, err := os.ReadDir(namesDir)
	if err != nil {
		v.Results.Errors = append(v.Results.Errors,
			fmt.Sprintf("error reading names directory: %v", err))
		return
	}

	if len(entries) == 0 {
		v.Results.Errors = append(v.Results.Errors, "no language files found in names directory")
		return
	}

	// Validate each language file
	foundValidLangFile := false
	for _, entry := range entries {
		if !entry.IsDir() && strings.HasSuffix(entry.Name(), ".toml") {
			langPath := filepath.Join(namesDir, entry.Name())
			var langConfig NameConfig
			if _, err := toml.DecodeFile(langPath, &langConfig); err != nil {
				v.Results.Errors = append(v.Results.Errors,
					fmt.Sprintf("error parsing language file %s: %v", entry.Name(), err))
				continue
			}

			foundValidLangFile = true

			// Check if major_arcana section exists
			if langConfig.MajorArcana == nil {
				v.Results.Warnings = append(v.Results.Warnings,
					fmt.Sprintf("missing [major_arcana] section in %s", entry.Name()))
			}

			// Check if minor_arcana sections exist
			if langConfig.MinorArcana == nil {
				v.Results.Warnings = append(v.Results.Warnings,
					fmt.Sprintf("missing [minor_arcana] section in %s", entry.Name()))
			}

			// Check if alt_text sections exist
			hasAltText := false
			if langConfig.MajorArcana != nil && langConfig.MajorArcana.AltText != nil {
				hasAltText = true
			}

			if !hasAltText && langConfig.MinorArcana != nil {
				for _, suit := range []string{"wands", "cups", "swords", "pentacles"} {
					suitConfig := langConfig.MinorArcana.GetSuit(suit)
					if suitConfig != nil && suitConfig.AltText != nil {
						hasAltText = true
						break
					}
				}
			}

			if !hasAltText {
				v.Results.Warnings = append(v.Results.Warnings,
					fmt.Sprintf("no alt_text sections found in %s", entry.Name()))
			}
		}
	}

	if !foundValidLangFile {
		v.Results.Errors = append(v.Results.Errors, "no valid language files found in names directory")
	}
}

func (v *Validator) validateAnsiArt() {
	// Find ANSI directories (ansi32, ansi256, etc.)
	entries, err := os.ReadDir(v.DeckPath)
	if err != nil {
		v.Results.Errors = append(v.Results.Errors,
			fmt.Sprintf("error reading deck directory: %v", err))
		return
	}

	foundAnsiDir := false
	for _, entry := range entries {
		if entry.IsDir() && strings.HasPrefix(entry.Name(), "ansi") {
			foundAnsiDir = true
			ansiDir := filepath.Join(v.DeckPath, entry.Name())
			v.validateAnsiDirectory(ansiDir, entry.Name())
		}
	}

	if !foundAnsiDir {
		v.Results.Warnings = append(v.Results.Warnings,
			"no ANSI art directories found (ansi32/, ansi256/, etc.)")
	}
}

// validateAnsiDirectory validates an ANSI art directory
func (v *Validator) validateAnsiDirectory(ansiDir, dirName string) {
	// Check for major_arcana directory
	majorArcanaDir := filepath.Join(ansiDir, "major_arcana")
	if _, err := os.Stat(majorArcanaDir); os.IsNotExist(err) {
		v.Results.Warnings = append(v.Results.Warnings,
			fmt.Sprintf("major_arcana directory not found in %s", dirName))
	} else {
		// Check for all 22 major arcana cards (00-21)
		missingCards := []string{}
		for i := 0; i <= 21; i++ {
			cardName := fmt.Sprintf("%02d", i)
			cardPath := filepath.Join(majorArcanaDir, cardName+".ansi")
			if _, err := os.Stat(cardPath); os.IsNotExist(err) {
				missingCards = append(missingCards, cardName)
			}
		}

		if len(missingCards) > 0 {
			v.Results.Warnings = append(v.Results.Warnings,
				fmt.Sprintf("missing ANSI art for major arcana cards in %s: %s",
					dirName, strings.Join(missingCards, ", ")))
		}
	}

	// Check for minor_arcana directory
	minorArcanaDir := filepath.Join(ansiDir, "minor_arcana")
	if _, err := os.Stat(minorArcanaDir); os.IsNotExist(err) {
		v.Results.Warnings = append(v.Results.Warnings,
			fmt.Sprintf("minor_arcana directory not found in %s", dirName))
	} else {
		// Check for all four suits
		suits := []string{"wands", "cups", "swords", "pentacles"}
		cardRanks := []string{
			"ace", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
			"page", "knight", "queen", "king",
		}

		for _, suit := range suits {
			suitDir := filepath.Join(minorArcanaDir, suit)
			if _, err := os.Stat(suitDir); os.IsNotExist(err) {
				v.Results.Warnings = append(v.Results.Warnings,
					fmt.Sprintf("missing suit directory: %s in %s/minor_arcana", suit, dirName))
				continue
			}

			// Check for all 14 cards in each suit
			missingCards := []string{}
			for _, rank := range cardRanks {
				cardPath := filepath.Join(suitDir, rank+".ansi")
				if _, err := os.Stat(cardPath); os.IsNotExist(err) {
					missingCards = append(missingCards, rank)
				}
			}

			if len(missingCards) > 0 {
				v.Results.Warnings = append(v.Results.Warnings,
					fmt.Sprintf("missing ANSI art for %s cards in %s: %s",
						suit, dirName, strings.Join(missingCards, ", ")))
			}
		}
	}
}

// Deck configuration structures
type DeckConfig struct {
	Deck             DeckSection               `toml:"deck"`
	CardBacks        *CardBackSection          `toml:"card_backs"`
	Aliases          *AliasSection             `toml:"aliases"`
	RemapMajorArcana map[string]string         `toml:"remap_major_arcana"`
	CustomCards      *CustomCardSection        `toml:"custom_cards"`
	Variants         map[string]VariantSection `toml:"variants"`
}

type DeckSection struct {
	ID            string               `toml:"id"`
	Name          string               `toml:"name"`
	Version       string               `toml:"version"`
	SchemaVersion string               `toml:"schema_version"`
	Icon          string               `toml:"icon"`
	Author        string               `toml:"author"`
	License       string               `toml:"license"`
	AspectRatio   float64              `toml:"aspect_ratio"`
	Description   string               `toml:"description"`
	CreatedDate   string               `toml:"created_date"`
	UpdatedDate   string               `toml:"updated_date"`
	Publisher     string               `toml:"publisher"`
	Website       string               `toml:"website"`
	Tags          []string             `toml:"tags"`
	ExcludedCards *ExcludedCardSection `toml:"excluded_cards"`
}

type ExcludedCardSection struct {
	Cards  []string `toml:"cards"`
	Reason string   `toml:"reason"`
}

type CardBackSection struct {
	Default  string                     `toml:"default"`
	Variants map[string]CardBackVariant `toml:"variants"`
}

type CardBackVariant struct {
	Name        string `toml:"name"`
	Image       string `toml:"image"`
	Description string `toml:"description"`
	AltText     string `toml:"alt_text"`
}

type AliasSection struct {
	Suits  map[string]string `toml:"suits"`
	Courts map[string]string `toml:"courts"`
}

type CustomCardSection struct {
	MajorArcana map[string]CustomCard               `toml:"major_arcana"`
	MinorArcana map[string]CustomMinorArcanaSection `toml:"minor_arcana"`
}

type CustomCard struct {
	ID       string `toml:"id"`
	Name     string `toml:"name"`
	Image    string `toml:"image"`
	AltText  string `toml:"alt_text"`
	Position int    `toml:"position"`
}

type CustomMinorArcanaSection struct {
	Name  string       `toml:"name"`
	Cards []CustomCard `toml:"cards"`
}

type VariantSection struct {
	ID          string `toml:"id"`
	Name        string `toml:"name"`
	CardBack    string `toml:"card_back"`
	Publisher   string `toml:"publisher"`
	CreatedDate string `toml:"created_date"`
}

// Name configuration structures
type NameConfig struct {
	MajorArcana *MajorArcanaNameSection `toml:"major_arcana"`
	MinorArcana *MinorArcanaNameSection `toml:"minor_arcana"`
	CardBacks   *CardBackNameSection    `toml:"card_backs"`
}

type MajorArcanaNameSection struct {
	Names   map[string]string `toml:"-"`
	AltText map[string]string `toml:"alt_text"`
}

type MinorArcanaNameSection struct {
	Wands     *SuitNameSection `toml:"wands"`
	Cups      *SuitNameSection `toml:"cups"`
	Swords    *SuitNameSection `toml:"swords"`
	Pentacles *SuitNameSection `toml:"pentacles"`
}

func (m *MinorArcanaNameSection) GetSuit(suit string) *SuitNameSection {
	switch suit {
	case "wands":
		return m.Wands
	case "cups":
		return m.Cups
	case "swords":
		return m.Swords
	case "pentacles":
		return m.Pentacles
	default:
		return nil
	}
}

type SuitNameSection struct {
	Names   map[string]string `toml:"-"`
	AltText map[string]string `toml:"alt_text"`
}

type CardBackNameSection struct {
	AltText map[string]string `toml:"alt_text"`
}
