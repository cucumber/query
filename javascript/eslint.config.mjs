import js from "@eslint/js";
import importPlugin from "eslint-plugin-import";
import n from "eslint-plugin-n";
import simpleImportSort from "eslint-plugin-simple-import-sort";
import globals from "globals";
import tseslint from "typescript-eslint";

export default tseslint.config(
    js.configs.recommended,
    tseslint.configs.recommended,
    importPlugin.flatConfigs.recommended,
    importPlugin.flatConfigs.typescript,
    {
        files: ["**/*.ts"],
        languageOptions: {
            ecmaVersion: 5,
            sourceType: "module",
            globals: {
                ...globals.browser,
                ...globals.node,
            },
        },
        plugins: {
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
);
