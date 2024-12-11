
#import "@preview/ilm:1.4.0": *
#import "@preview/gentle-clues:1.0.0": *
#import "@preview/codly:1.0.0": *

#show: ilm.with(
  title: [Rapport Labo 6],
  author: "Ali Zoubir & Kenan Augsburger",
  date: datetime.today(),
)

#codly(
  languages: (
    cpp: (
      name: "cpp",
      icon: text(font: "JetBrainsMono NFP", " "),
      color: rgb("#ef4444")
    )
  )
)

#show: codly-init.with()

#show link: underline
#show raw: set text(font: "JetBrainsMono NFP")

= Informations sur le projet

== Répertoire Git (sur GitHub)

#link("https://github.com/Mondotosz/pco_lab06")[mondotosz/pco_lab06]

== Membres

#list[
  #link("https://github.com/Ali-Z0")[Ali Zoubir]
][
  #link("https://github.com/mondotosz")[Kénan Augsburger]
]

== Introduction du problème

#task([
  todo
])

= Implémentation et choix

== Architecture des classes

#task([
  todo
])

== Méthodes clés

#task([
  todo
])

= Tests

#task([
  todo
])

= Remarques et conclusion

== Remarques

#task([
  todo
])

== Conclusion

#task([
  todo
])

