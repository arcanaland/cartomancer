package deck

import (
	"fmt"
	"os"
	"path/filepath"
	"strings"

	"github.com/BurntSushi/toml"
	"github.com/arcanaland/cartomancer/internal/card"
)

// Deck represents a tarot deck
type Deck struct {
	ID          string
	Name        string
	Version     string
	Author      string
	Description string
	Path        string

	// Card maps for lookup
	MajorArcana map[string]*card.Card
	MinorArcana map[string]map[string]*card.Card

	// Raw config data
	config *DeckConfig
}

// LoadDeck loads a tarot deck from a directory
func LoadDeck(deckPath string) (*Deck, error) {
	// Check if deck.toml exists
	deckTomlPath := filepath.Join(deckPath, "deck.toml")
	if _, err := os.Stat(deckTomlPath); os.IsNotExist(err) {
		return nil, fmt.Errorf("deck.toml not found in %s", deckPath)
	}

	// Decode deck.toml
	var config DeckConfig
	if _, err := toml.DecodeFile(deckTomlPath, &config); err != nil {
		return nil, fmt.Errorf("error parsing deck.toml: %v", err)
	}

	// Create deck
	deck := &Deck{
		ID:          config.Deck.ID,
		Name:        config.Deck.Name,
		Version:     config.Deck.Version,
		Author:      config.Deck.Author,
		Description: config.Deck.Description,
		Path:        deckPath,
		MajorArcana: make(map[string]*card.Card),
		MinorArcana: make(map[string]map[string]*card.Card),
		config:      &config,
	}

	// Load card names and alt text
	if err := deck.loadCardInfo(); err != nil {
		return nil, fmt.Errorf("error loading card info: %v", err)
	}

	return deck, nil
}

// loadCardInfo loads card names and alt text from the names directory
func (d *Deck) loadCardInfo() error {
	// Create cards for major arcana (00-21)
	for i := 0; i <= 21; i++ {
		cardNumber := fmt.Sprintf("%02d", i)
		cardID := fmt.Sprintf("major_arcana.%s", cardNumber)

		c := &card.Card{
			ID:     cardID,
			Type:   "major_arcana",
			Number: cardNumber,
		}

		d.MajorArcana[cardNumber] = c
	}

	// Create cards for minor arcana
	suits := []string{"wands", "cups", "swords", "pentacles"}
	ranks := []string{
		"ace", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten",
		"page", "knight", "queen", "king",
	}

	for _, suit := range suits {
		d.MinorArcana[suit] = make(map[string]*card.Card)

		for _, rank := range ranks {
			cardID := fmt.Sprintf("minor_arcana.%s.%s", suit, rank)

			c := &card.Card{
				ID:   cardID,
				Type: "minor_arcana",
				Suit: suit,
				Rank: rank,
			}

			d.MinorArcana[suit][rank] = c
		}
	}

	// Try to load names and alt text
	namesDir := filepath.Join(d.Path, "names")
	if _, err := os.Stat(namesDir); os.IsNotExist(err) {
		// No names directory, use default names
		d.setDefaultNames()
		return nil
	}

	// Try to load english names first
	enTomlPath := filepath.Join(namesDir, "en.toml")
	if _, err := os.Stat(enTomlPath); os.IsNotExist(err) {
		// No english names, check for any other language file
		entries, err := os.ReadDir(namesDir)
		if err != nil || len(entries) == 0 {
			// No language files, use default names
			d.setDefaultNames()
			return nil
		}

		// Use the first language file found
		for _, entry := range entries {
			if !entry.IsDir() && filepath.Ext(entry.Name()) == ".toml" {
				enTomlPath = filepath.Join(namesDir, entry.Name())
				break
			}
		}
	}

	// Decode language file
	var langConfig NameConfig
	if _, err := toml.DecodeFile(enTomlPath, &langConfig); err != nil {
		// Error parsing language file, use default names
		d.setDefaultNames()
		return fmt.Errorf("error parsing language file: %v", err)
	}

	// Set names and alt text from language file
	if langConfig.MajorArcana != nil {
		for num, name := range langConfig.MajorArcana {
			if card, ok := d.MajorArcana[num]; ok {
				card.Name = name
			}
		}

		if langConfig.MajorArcanaAltText != nil {
			for num, altText := range langConfig.MajorArcanaAltText {
				if card, ok := d.MajorArcana[num]; ok {
					card.AltText = altText
				}
			}
		}
	}

	if langConfig.MinorArcana != nil {
		for suit, ranks := range langConfig.MinorArcana {
			if suitMap, ok := d.MinorArcana[suit]; ok {
				for rank, name := range ranks {
					if card, ok := suitMap[rank]; ok {
						card.Name = name
					}
				}
			}
		}

		if langConfig.MinorArcanaAltText != nil {
			for suit, ranks := range langConfig.MinorArcanaAltText {
				if suitMap, ok := d.MinorArcana[suit]; ok {
					for rank, altText := range ranks {
						if card, ok := suitMap[rank]; ok {
							card.AltText = altText
						}
					}
				}
			}
		}
	}

	// If any card is missing a name, set default name
	for _, card := range d.MajorArcana {
		if card.Name == "" {
			card.Name = getDefaultMajorArcanaName(card.Number)
		}
	}

	for _, suitMap := range d.MinorArcana {
		for _, card := range suitMap {
			if card.Name == "" {
				card.Name = getDefaultMinorArcanaName(card.Rank, card.Suit)
			}
		}
	}

	return nil
}

// setDefaultNames sets default names for all cards
func (d *Deck) setDefaultNames() {
	// Set default names for major arcana
	for num, card := range d.MajorArcana {
		card.Name = getDefaultMajorArcanaName(num)
	}

	// Set default names for minor arcana
	for suit, suitMap := range d.MinorArcana {
		for rank, card := range suitMap {
			card.Name = getDefaultMinorArcanaName(rank, suit)
		}
	}
}

// GetCard gets a card by its canonical ID
func (d *Deck) GetCard(cardID string) (*card.Card, error) {
	parts := splitCardID(cardID)
	if len(parts) < 2 {
		return nil, fmt.Errorf("invalid card ID format: %s", cardID)
	}

	if parts[0] == "major_arcana" && len(parts) == 2 {
		// Major arcana card
		card, ok := d.MajorArcana[parts[1]]
		if !ok {
			return nil, fmt.Errorf("card not found: %s", cardID)
		}
		return card, nil
	} else if parts[0] == "minor_arcana" && len(parts) == 3 {
		// Minor arcana card
		suitMap, ok := d.MinorArcana[parts[1]]
		if !ok {
			return nil, fmt.Errorf("suit not found: %s", parts[1])
		}
		card, ok := suitMap[parts[2]]
		if !ok {
			return nil, fmt.Errorf("card not found: %s", cardID)
		}
		return card, nil
	}

	return nil, fmt.Errorf("invalid card ID format: %s", cardID)
}

// Helper functions

// splitCardID splits a canonical card ID into parts
func splitCardID(cardID string) []string {
	return strings.Split(cardID, ".")
}

// getDefaultMajorArcanaName returns the default name for a major arcana card
func getDefaultMajorArcanaName(number string) string {
	names := map[string]string{
		"00": "The Fool",
		"01": "The Magician",
		"02": "The High Priestess",
		"03": "The Empress",
		"04": "The Emperor",
		"05": "The Hierophant",
		"06": "The Lovers",
		"07": "The Chariot",
		"08": "Strength",
		"09": "The Hermit",
		"10": "Wheel of Fortune",
		"11": "Justice",
		"12": "The Hanged Man",
		"13": "Death",
		"14": "Temperance",
		"15": "The Devil",
		"16": "The Tower",
		"17": "The Star",
		"18": "The Moon",
		"19": "The Sun",
		"20": "Judgement",
		"21": "The World",
	}

	if name, ok := names[number]; ok {
		return name
	}

	return fmt.Sprintf("Major Arcana %s", number)
}

// getDefaultMinorArcanaName returns the default name for a minor arcana card
func getDefaultMinorArcanaName(rank, suit string) string {
	// Capitalize the first letter of suit
	capitalizedSuit := strings.ToUpper(suit[:1]) + suit[1:]

	// Get rank name
	var rankName string
	switch rank {
	case "ace", "two", "three", "four", "five", "six", "seven", "eight", "nine", "ten":
		rankName = strings.Title(rank)
	default:
		rankName = strings.Title(rank)
	}

	return fmt.Sprintf("%s of %s", rankName, capitalizedSuit)
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
	MajorArcana        map[string]string            `toml:"major_arcana"`
	MajorArcanaAltText map[string]string            `toml:"major_arcana.alt_text"`
	MinorArcana        map[string]map[string]string `toml:"minor_arcana"`
	MinorArcanaAltText map[string]map[string]string `toml:"minor_arcana.alt_text"`
}
