package cmd

import (
	"fmt"
	"os"

	"github.com/arcanaland/cartomancer/internal/validator"
	"github.com/spf13/cobra"
)

// validateCmd represents the validate command
var validateCmd = &cobra.Command{
	Use:   "validate [path]",
	Short: "Validate a tarot deck directory",
	Long: `Validate checks if a tarot deck directory conforms to the Tarot Deck Specification v1.0.
It verifies the structure, required files, and conformity to the specification.`,
	Args: cobra.ExactArgs(1),
	RunE: func(cmd *cobra.Command, args []string) error {
		deckPath := args[0]

		// Check if path exists
		if _, err := os.Stat(deckPath); os.IsNotExist(err) {
			return fmt.Errorf("deck directory not found: %s", deckPath)
		}

		// Create validator and run validation
		v := validator.NewValidator(deckPath)
		results, err := v.Validate()
		if err != nil {
			return fmt.Errorf("validation error: %v", err)
		}

		// Display validation results
		fmt.Println("Validation Results:")
		fmt.Println("-------------------")
		
		if len(results.Errors) == 0 {
			fmt.Printf("✅ Deck '%s' is valid according to the specification.\n", deckPath)
		} else {
			fmt.Printf("❌ Deck '%s' has %d validation errors:\n", deckPath, len(results.Errors))
			for i, err := range results.Errors {
				fmt.Printf("%d. %s\n", i+1, err)
			}
			return fmt.Errorf("validation failed")
		}

		if len(results.Warnings) > 0 {
			fmt.Println("\nWarnings:")
			for i, warn := range results.Warnings {
				fmt.Printf("%d. %s\n", i+1, warn)
			}
		}

		return nil
	},
}