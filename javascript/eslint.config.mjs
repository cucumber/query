import js from "@eslint/js";
import importPlugin from "eslint-plugin-import";
import n from "eslint-plugin-n";
import simpleImportSort from "eslint-plugin-simple-import-sort";
import globals from "globals";
import typescriptEslintPlugin from "@typescript-eslint/eslint-plugin";
import typescriptParser from "@typescript-eslint/parser";

export default [
    js.configs.recommended,
    ...typescriptEslintPlugin.configs["flat/recommended"],
    importPlugin.flatConfigs.recommended,
    importPlugin.flatConfigs.typescript,
    {
        files: ["**/*.ts"],
        languageOptions: {
            parser: typescriptParser,
            ecmaVersion: 5,
            sourceType: "module",
            globals: {
                ...globals.browser,
                ...globals.node,
            },
        },
        plugins: {
            "@typescript-eslint": typescriptEslintPlugin,
            n,
            "simple-import-sort": simpleImportSort,
        },
        rules: {
            "import/no-cycle": "error",
            "n/no-extraneous-import": "error",
            "@typescript-eslint/no-explicit-any": "error",
            "@typescript-eslint/no-non-null-assertion": "error",
            "simple-import-sort/imports": "error",
            "simple-import-sort/exports": "error",
        },
    },
    {
        files: ["src/**/*.spec.ts"],
        rules: {
            "@typescript-eslint/no-non-null-assertion": "off",
        },
    },
];
