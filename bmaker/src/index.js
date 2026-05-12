export default {
  async fetch(request) {
    return new Response("IIDX Worker OK", {
      headers: { "content-type": "text/plain" },
    });
  },
};