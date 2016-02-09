package main

import (
	"fmt"
	"github.com/gorilla/feeds"
	"github.com/gorilla/mux"
	"net/http"
	"os"
	"log"
	"time"
)

func handler(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "application/rss+xml; charset=utf-8")

	now := time.Now()

	feed := &feeds.Feed{
		Title:       "title text",
		Link:        &feeds.Link{ Href: "https://www.google.ru" },
		Description: "description text",
		Author:      &feeds.Author{ "Alexey K", "alexey@gmail.com" },
		Created:     now,
	}

	feed.Items = []*feeds.Item{
		&feeds.Item{
			Title:       "Stored procedures in Redis",
			Link:        &feeds.Link{ Href: "https://habrahabr.ru/post/270251" },
			Description: "Create and manage lua stored procedures in Redis NoSql database",
			Author:      &feeds.Author{ "Alexey K", "alexey@gmail.com" },
			Created:     now,
		},
		&feeds.Item{
			Title:       "Amazon Web Services is now part of Github Students Development Pack",
			Link:        &feeds.Link{ Href: "https://habrahabr.ru/post/270123" },
			Description: "",
			Author:      &feeds.Author{ "Alexey K", "alexey@gmail.com" },
			Created:     now,
		},
	}

	rss, err := feed.ToRss()
	if err != nil {
		log.Fatal(err)
	}

	fmt.Fprintf(w, "%v", rss)
}

func handlerPartialMissing(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "application/rss+xml; charset=utf-8")

//   	now := time.Now()

	feed := &feeds.Feed{
		Title:       "RSS test server",
		Link:        &feeds.Link{ Href: "http://localhost/missing" },
		Description: "description text",
		Author:      &feeds.Author{ "Alexey K", "alexey@gmail.com" },
//  		Created:     now,
	}

	feed.Items = []*feeds.Item{
		&feeds.Item{
			Title:       "Stored procedures in Redis",
			Description: "Create and manage lua stored procedures in Redis NoSql database",
			Link:        &feeds.Link{ Href: "https://habrahabr.ru/post/270251" },
			Author:      &feeds.Author{ "Alexey K", "alexey@gmail.com" },
// 			Created:     now,
		},
		&feeds.Item{
			Title:       "Amazon Web Services is now part of Github Students Development Pack",
			Link:        &feeds.Link{ Href: "https://habrahabr.ru/post/270123" },
			Description: "",
			Author:      &feeds.Author{ "Alexey K", "alexey@gmail.com" },
		},
	}

	rss, err := feed.ToRss()
	if err != nil {
		log.Fatal(err)
	}

	fmt.Fprintf(w, "%v", rss)
}

func handlerBrokenRss(w http.ResponseWriter, _ *http.Request) {
	w.Header().Set("Content-Type", "application/rss+xml; charset=utf-8")

	fmt.Fprintf(w, "%s",
`<?xml version="1.0" encoding="UTF-8"?><rss version="2.0">
  <channel>
    <title>title text</title>
    <link>https://www.google.ru</link>
    <description>description text</description>
    <managingEditor>alexey@gmail.com (Alexey K)</managingEditor>
    <pubDate>Tue, 09 Feb 2016 21:09:09 +0300</pubDate>
    <item>`)
}

func handlerTimeout(w http.ResponseWriter, r *http.Request) {
	time.Sleep(2000 * time.Millisecond)
	handler(w, r);
}

func main() {
	args := os.Args[1:]

	port := ":" + args[0]

	router := mux.NewRouter()
	router.HandleFunc("/", handler).Methods("GET")
	router.HandleFunc("/missing", handlerPartialMissing).Methods("GET")
	router.HandleFunc("/timeout", handlerTimeout).Methods("GET")
	router.HandleFunc("/broken", handlerBrokenRss).Methods("GET")

	server := &http.Server {
		Addr: port,
		Handler: router,
	}

	err := server.ListenAndServe()
	if err != nil {
		log.Fatal(err)
	}
}