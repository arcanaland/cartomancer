package card

// Card represents a tarot card
type Card struct {
	ID      string // Canonical ID (e.g., major_arcana.00, minor_arcana.wands.ace)
	Name    string // Localized name
	Type    string // major_arcana or minor_arcana
	Number  string // For major arcana (00-21)
	Suit    string // For minor arcana (wands, cups, swords, pentacles)
	Rank    string // For minor arcana (ace, two, ..., king)
	AltText string // Descriptive alt text
}