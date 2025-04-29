package cmd

import (
	"github.com/spf13/cobra"
)

// RootCmd represents the base command when called without any subcommands
var RootCmd = &cobra.Command{
	Use:   "cartomancer",
	Short: "Tool for validating and managing tarot decks",
	Long: `Cartomancer is a command-line tool for validating, and managing tarot decks and esoterica.
It helps ensure that decks conform to the Tarot Deck Specification v1.0 maintained by Arcana Land.`,
}

func init() {
	RootCmd.AddCommand(validateCmd)
}

// Execute adds all child commands to the root command and sets flags appropriately.
func Execute() error {
	return RootCmd.Execute()
}
