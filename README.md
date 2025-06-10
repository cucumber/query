<h1 align="center">
  <img src="https://raw.githubusercontent.com/cucumber/cucumber-js/7df2c9b4f04099b81dc5c00cd73b404401cd6e46/docs/images/logo.svg" alt="">
  <br>
  Cucumber Query
</h1>
<p align="center">
  <b>Given one <a href="https://github.com/cucumber/messages">Cucumber Message</a>, find another</b>
</p>

<p align="center">
  <a href="https://www.npmjs.com/package/@cucumber/query">
    <img src="https://img.shields.io/npm/v/@cucumber/query.svg?color=dark-green" alt="npm">
  </a>
  <a href="https://central.sonatype.com/artifact/io.cucumber/query">
    <img src="https://img.shields.io/maven-central/v/io.cucumber/query.svg?label=Maven%20Central&color=dark-green" alt="maven-central">
  </a>
  <a href="https://github.com/cucumber/query/actions/workflows/release-github.yaml">
    <img src="https://github.com/cucumber/query/actions/workflows/release-github.yaml/badge.svg" alt="build">
  </a>
  <a href="https://opencollective.com/cucumber">
    <img src="https://opencollective.com/cucumber/backers/badge.svg" alt="backers">
  </a>
  <a href="https://opencollective.com/cucumber">
    <img src="https://opencollective.com/cucumber/sponsors/badge.svg" alt="sponsors">
  </a>
</p>

## Overview

The different message types in `cucumber-messages` have references to each other
using `id` fields. It's a bit similar to rows in a relational database, with
primary and foreign keys.

Consumers of these messages may want to *query* the messages for certain information.
For example, [@cucumber/react-components](https://github.com/cucumber/react-components) needs to know the status of
a [Step](https://github.com/cucumber/messages/blob/main/messages.md#step) as it
is rendering the [GherkinDocument](https://github.com/cucumber/messages/blob/main/messages.md#gherkindocument)

The Query library makes this easy by providing a function to look up the
status of a step, a scenario or an entire file.
